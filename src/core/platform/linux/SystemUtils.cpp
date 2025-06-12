#include "core/SystemUtils.h"

#include <linux/limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <mutex>
#include <regex>

#include "core/Logger.h"
#include "nlohmann/json.hpp"

namespace SystemUtils {

// 终端会话管理
struct TerminalSession {
    pid_t pid;
    int inputFd;
    int outputFd;
    std::chrono::steady_clock::time_point lastUsed;
    bool isActive;
};

static std::map<std::string, TerminalSession> g_sessions;
static std::mutex g_sessionsMutex;
static const auto SESSION_TIMEOUT = std::chrono::minutes(10);  // 10分钟超时
static size_t g_maxOutputLength = 8192;  // 默认最大输出长度 8KB

// 清理ANSI转义序列和多余的格式字符，并限制输出长度
std::string cleanOutput(const std::string& rawOutput) {
    std::string cleaned = rawOutput;

    // 移除ANSI转义序列 (如 \u001b[32;1m, \u001b[0m 等)
    std::regex ansiRegex(R"(\x1b\[[0-9;]*[mK])");
    cleaned = std::regex_replace(cleaned, ansiRegex, "");

    // 移除多余的空行 (连续的\r?\n)
    std::regex multiNewlineRegex(R"((\r?\n){3,})");
    cleaned = std::regex_replace(cleaned, multiNewlineRegex, "\n\n");

    // 移除开头和结尾的空白字符
    cleaned = std::regex_replace(cleaned, std::regex(R"(^\s+|\s+$)"),
                                 "");  // 限制输出长度
    if (cleaned.length() > g_maxOutputLength) {
        // 截取前面部分，并添加截断提示
        cleaned =
            cleaned.substr(0, g_maxOutputLength - 100);  // 预留空间给提示信息

        // 在最后一个完整行处截断
        size_t lastNewline = cleaned.find_last_of('\n');
        if (lastNewline != std::string::npos &&
            lastNewline > g_maxOutputLength / 2) {
            cleaned = cleaned.substr(0, lastNewline);
        }

        cleaned +=
            "\n\n[输出过长，已截断。使用 'head' 或 'tail' 命令查看特定部分]";
    }

    return cleaned;
}

void enableHighDPI() {}

void addToStartup(const std::string& appName) {
    std::string exePath = getExecutablePath();
    std::string autostartDir =
        std::format("{}/.config/autostart", std::getenv("HOME"));
    std::filesystem::create_directories(autostartDir);  // 确保目录存在

    std::string desktopEntryPath =
        std::format("{}/{}.desktop", autostartDir, appName);

    std::ofstream out(desktopEntryPath);
    out << "[Desktop Entry]\n";
    out << "Version=1.0\n";
    out << "Type=Application\n";
    out << std::format("Name={}\n", appName);
    out << std::format("Exec={}\n", exePath);
    out << "Terminal=true\n";  // 打开终端运行
    out << "Hidden=false\n";
    out << "NoDisplay=false\n";
    out << "X-GNOME-Autostart-enabled=true\n";
    out.close();
}

void removeFromStartup(const std::string& appName) {
    std::string autostartDir =
        std::format("{}/.config/autostart", std::getenv("HOME"));
    std::string desktopEntryPath =
        std::format("{}/{}.desktop", autostartDir, appName);
    std::filesystem::remove(desktopEntryPath);
}

std::string getExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count <= 0 || count == PATH_MAX) return "";
    return std::string(result, count);
}

std::string getExecutableDir() {
    return std::filesystem::path(getExecutablePath()).parent_path().string();
}

nlohmann::json createTerminalSession(const std::string& session_id) {
    nlohmann::json result;
    std::lock_guard<std::mutex> lock(g_sessionsMutex);

    try {
        // 检查会话是否已存在
        if (g_sessions.find(session_id) != g_sessions.end()) {
            result["success"] = true;
            result["message"] = "Session already exists";
            result["session_id"] = session_id;
            return result;
        }

        // 创建管道
        int inputPipe[2], outputPipe[2];
        if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1) {
            result["success"] = false;
            result["error"] = "Failed to create pipes";
            return result;
        }

        // 创建子进程
        pid_t pid = fork();
        if (pid == -1) {
            close(inputPipe[0]);
            close(inputPipe[1]);
            close(outputPipe[0]);
            close(outputPipe[1]);
            result["success"] = false;
            result["error"] = "Failed to fork process";
            return result;
        }

        if (pid == 0) {
            // 子进程 - bash终端
            close(inputPipe[1]);   // 关闭写端
            close(outputPipe[0]);  // 关闭读端

            // 重定向标准输入输出
            dup2(inputPipe[0], STDIN_FILENO);
            dup2(outputPipe[1], STDOUT_FILENO);
            dup2(outputPipe[1], STDERR_FILENO);

            // 关闭原管道文件描述符
            close(inputPipe[0]);
            close(outputPipe[1]);

            // 启动bash
            execl("/bin/bash", "bash", "--norc", "--noprofile", "-i", nullptr);
            exit(1);  // 如果execl失败
        } else {
            // 父进程
            close(inputPipe[0]);   // 关闭读端
            close(outputPipe[1]);  // 关闭写端

            // 保存会话信息
            TerminalSession session;
            session.pid = pid;
            session.inputFd = inputPipe[1];
            session.outputFd = outputPipe[0];
            session.lastUsed = std::chrono::steady_clock::now();
            session.isActive = true;

            g_sessions[session_id] = session;

            result["success"] = true;
            result["message"] = "Terminal session created successfully";
            result["session_id"] = session_id;

            Logger::info(
                std::format("Created terminal session: {}", session_id));
        }

    } catch (const std::exception& e) {
        result["success"] = false;
        result["error"] = e.what();
    }

    return result;
}

nlohmann::json closeTerminalSession(const std::string& session_id) {
    nlohmann::json result;
    std::lock_guard<std::mutex> lock(g_sessionsMutex);

    auto it = g_sessions.find(session_id);
    if (it == g_sessions.end()) {
        result["success"] = false;
        result["error"] = "Session not found";
        return result;
    }

    try {
        TerminalSession& session = it->second;

        // 关闭文件描述符
        if (session.inputFd != -1) close(session.inputFd);
        if (session.outputFd != -1) close(session.outputFd);

        // 终止子进程
        if (session.pid > 0) {
            kill(session.pid, SIGTERM);
            waitpid(session.pid, nullptr, 0);  // 等待子进程结束
        }

        g_sessions.erase(it);

        result["success"] = true;
        result["message"] = "Terminal session closed successfully";
        result["session_id"] = session_id;

        Logger::info(std::format("Closed terminal session: {}", session_id));

    } catch (const std::exception& e) {
        result["success"] = false;
        result["error"] = e.what();
    }

    return result;
}

nlohmann::json listActiveSessions() {
    nlohmann::json result;
    std::lock_guard<std::mutex> lock(g_sessionsMutex);

    nlohmann::json sessions = nlohmann::json::array();
    auto now = std::chrono::steady_clock::now();

    for (const auto& [session_id, session] : g_sessions) {
        nlohmann::json sessionInfo;
        sessionInfo["session_id"] = session_id;
        sessionInfo["is_active"] = session.isActive;

        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
            now - session.lastUsed);
        sessionInfo["idle_minutes"] = elapsed.count();

        sessions.push_back(sessionInfo);
    }

    result["success"] = true;
    result["sessions"] = sessions;
    return result;
}

void cleanupTimeoutSessions() {
    std::lock_guard<std::mutex> lock(g_sessionsMutex);
    auto now = std::chrono::steady_clock::now();

    auto it = g_sessions.begin();
    while (it != g_sessions.end()) {
        auto elapsed = now - it->second.lastUsed;
        if (elapsed > SESSION_TIMEOUT) {
            Logger::info(
                std::format("Cleaning up timeout session: {}", it->first));

            // 关闭文件描述符和进程
            if (it->second.inputFd != -1) close(it->second.inputFd);
            if (it->second.outputFd != -1) close(it->second.outputFd);
            if (it->second.pid > 0) {
                kill(it->second.pid, SIGTERM);
                waitpid(it->second.pid, nullptr, WNOHANG);  // 非阻塞等待
            }

            it = g_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

nlohmann::json executeShellCommand(const std::string& command,
                                   const std::string& session_id) {
    nlohmann::json result;

    try {
        Logger::info(std::format("Executing shell command: {} (session: {})",
                                 command, session_id));  // 清理超时会话
        cleanupTimeoutSessions();

        std::unique_lock<std::mutex> lock(g_sessionsMutex);
        // 查找现有会话
        auto it = g_sessions.find(session_id);
        if (it == g_sessions.end()) {
            // 如果会话不存在，自动创建一个新会话
            Logger::info(
                std::format("Session {} not found, creating new session",
                            session_id));  // 释放锁来创建会话
            lock.unlock();

            // 创建新会话
            auto createResult = createTerminalSession(session_id);
            if (!createResult["success"]) {
                Logger::error(
                    std::format("Failed to create session {}: {}", session_id,
                                createResult["error"].get<std::string>()));

                // 如果创建会话失败，回退到一次性执行
                std::string full_command =
                    std::format("bash -c \"{}\"", command);
                FILE* pipe = popen(full_command.c_str(), "r");
                if (!pipe) {
                    result["success"] = false;
                    result["error"] = "Failed to execute command";
                    result["stderr"] = "Could not create process";
                    result["exit_code"] = -1;
                    result["command"] = command;
                    result["session_id"] = session_id;
                    result["finished"] = true;
                    result["reused_session"] = false;
                    result["timestamp"] =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
                    return result;
                }

                std::string output;
                char buffer[4096];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }

                int exit_code = pclose(pipe);
                result["success"] = (exit_code == 0);
                result["command"] = command;
                result["session_id"] = session_id;
                result["stdout"] = output;
                result["stderr"] = "";
                result["exit_code"] = exit_code;
                result["finished"] = true;
                result["reused_session"] = false;
                result["timestamp"] =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();

                Logger::info(std::format(
                    "Command executed successfully. Exit code: {}", exit_code));
                return result;
            }
            // 重新获取锁并查找新创建的会话
            lock.lock();
            it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                result["success"] = false;
                result["error"] = "Failed to find newly created session";
                return result;
            }
        }

        TerminalSession& session = it->second;

        // 检查进程是否还活着
        int status;
        pid_t check_result = waitpid(session.pid, &status, WNOHANG);
        if (check_result != 0) {
            // 进程已死，移除会话并回退到一次性执行
            Logger::info(std::format(
                "Session {} process is dead, removing session", session_id));

            // 清理死掉的会话
            if (session.inputFd != -1) close(session.inputFd);
            if (session.outputFd != -1) close(session.outputFd);
            g_sessions.erase(it);
            // 释放锁并回退到一次性执行
            lock.unlock();

            std::string full_command = std::format("bash -c \"{}\"", command);
            FILE* pipe = popen(full_command.c_str(), "r");
            if (!pipe) {
                result["success"] = false;
                result["error"] = "Failed to execute command";
                result["reused_session"] = false;
                return result;
            }

            std::string output;
            char buffer[4096];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                output += buffer;
            }

            int exit_code = pclose(pipe);
            result["success"] = (exit_code == 0);
            result["stdout"] = output;
            result["exit_code"] = exit_code;
            result["reused_session"] = false;
            result["command"] = command;
            result["session_id"] = session_id;
            result["finished"] = true;
            result["timestamp"] =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            return result;
        }

        // 更新最后使用时间
        session.lastUsed = std::chrono::steady_clock::now();

        // 向终端写入命令
        std::string commandWithNewline =
            command + "\necho \"<<<COMMAND_END>>>\"\n";
        ssize_t written = write(session.inputFd, commandWithNewline.c_str(),
                                commandWithNewline.length());
        if (written == -1) {
            result["success"] = false;
            result["error"] = "Failed to write command to terminal";
            result["reused_session"] = true;
            result["command"] = command;
            result["session_id"] = session_id;
            result["finished"] = true;
            result["timestamp"] =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            return result;
        }

        // 读取输出直到看到结束标记
        std::string output;
        char buffer[4096];
        ssize_t bytesRead;
        bool foundEnd = false;
        auto startTime = std::chrono::steady_clock::now();
        const auto timeout =
            std::chrono::seconds(30);  // 30秒超时        while (!foundEnd) {
        // 检查超时
        if (std::chrono::steady_clock::now() - startTime > timeout) {
            result["success"] = false;
            result["error"] = "Command execution timeout";
            result["reused_session"] = true;
            result["command"] = command;
            result["session_id"] = session_id;
            result["finished"] = true;
            result["timestamp"] =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            return result;
        }  // 检查输出长度限制
        if (output.length() > g_maxOutputLength) {
            Logger::info(std::format(
                "Output length exceeded limit for session: {}", session_id));
            foundEnd = true;
            output += "\n\n[输出过长，已提前终止读取]";
            break;
        }

        // 使用select实现超时读取
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(session.outputFd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms

        int selectResult =
            select(session.outputFd + 1, &readfds, nullptr, nullptr, &tv);
        if (selectResult > 0 && FD_ISSET(session.outputFd, &readfds)) {
            bytesRead = read(session.outputFd, buffer, sizeof(buffer) - 1);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                output += buffer;

                // 检查是否找到结束标记
                if (output.find("<<<COMMAND_END>>>") != std::string::npos) {
                    foundEnd = true;
                    // 移除结束标记及其后的内容
                    size_t endPos = output.find("<<<COMMAND_END>>>");
                    if (endPos != std::string::npos) {
                        output = output.substr(0, endPos);
                    }
                }
            }
        }
    }  // 构建结果
    result["success"] = true;
    result["command"] = command;
    result["session_id"] = session_id;
    result["stdout"] = cleanOutput(output);
    result["stderr"] = "";
    result["exit_code"] = 0;
    result["finished"] = true;
    result["reused_session"] = true;
    result["timestamp"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    Logger::info(std::format("Command executed successfully in session: {}",
                             session_id));
}
catch (const std::exception& e) {
    Logger::error(
        std::format("Exception while executing command: {}", e.what()));
    result["success"] = false;
    result["error"] = e.what();
    result["command"] = command;
    result["session_id"] = session_id;
    result["stdout"] = "";
    result["stderr"] = e.what();
    result["exit_code"] = -1;
    result["finished"] = true;
    result["reused_session"] = false;
    result["timestamp"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
}

return result;
}

void setOutputLengthLimit(size_t limit) {
    std::lock_guard<std::mutex> lock(g_sessionsMutex);
    g_maxOutputLength = limit;
    Logger::info(std::format("Output length limit set to: {} bytes", limit));
}

size_t getOutputLengthLimit() {
    std::lock_guard<std::mutex> lock(g_sessionsMutex);
    return g_maxOutputLength;
}

}  // namespace SystemUtils
