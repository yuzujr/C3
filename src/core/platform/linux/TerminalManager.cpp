#include "core/TerminalManager.h"

#include <signal.h>
#include <sys/wait.h>

#include <chrono>
#include <format>
#include <map>
#include <mutex>
#include <regex>
#include <sstream>
#include <vector>

#include "core/Logger.h"

namespace TerminalManager {
// 终端会话管理
struct TerminalSession {
    pid_t pid;
    int inputFd;
    int outputFd;
    std::chrono::steady_clock::time_point lastUsed;
};

static std::map<std::string, TerminalSession> g_sessions;
static std::mutex g_sessionsMutex;
static const auto SESSION_TIMEOUT = std::chrono::minutes(10);  // 10分钟超时
static size_t g_maxOutputLength = 8192;  // 默认最大输出长度 8KB

// 清理ANSI转义序列和多余的格式字符，并限制输出长度
std::string cleanOutput(const std::string& rawOutput,
                        const std::string& originalCommand = "") {
    std::string cleaned = rawOutput;

    // 移除ANSI转义序列 (如 \u001b[32;1m, \u001b[0m 等)
    std::regex ansiRegex(R"(\x1b\[[0-9;]*[mK])");
    cleaned = std::regex_replace(cleaned, ansiRegex, "");

    // 移除回车符
    cleaned = std::regex_replace(cleaned, std::regex("\r"), "");

    // 按行分割并处理
    std::vector<std::string> lines;
    std::stringstream ss(cleaned);
    std::string line;

    while (std::getline(ss, line)) {
        // 移除行尾空白
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }

        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 如果包含原始命令，跳过这一行（命令回显）
        if (!originalCommand.empty() &&
            line.find(originalCommand) != std::string::npos) {
            continue;
        }

        // 移除bash提示符模式 (例如: "684c2a4b-393d-2903 [~] > ", "bash-5.2# ",
        // etc.)
        std::regex promptPatterns[] = {
            std::regex(
                R"(^[a-zA-Z0-9\-_]+ \[.*?\] > )"),  // "session_id [path] > "
            std::regex(R"(^bash-[0-9.]+[#$] )"),    // "bash-5.2# "
            std::regex(R"(^[^@]*@[^:]*:[^$#]*[#$] )"),  // "user@host:path$ "
            std::regex(R"(^[^$#%>]*[#$%>] )"),          // 通用提示符
        };

        bool isPromptLine = false;
        for (const auto& pattern : promptPatterns) {
            if (std::regex_search(line, pattern)) {
                // 这是一个提示符行，检查是否只有提示符
                std::string afterPrompt = std::regex_replace(line, pattern, "");
                if (afterPrompt.empty() || afterPrompt == originalCommand) {
                    isPromptLine = true;
                    break;
                }
                // 如果提示符后有其他内容，去掉提示符但保留内容
                line = afterPrompt;
                break;
            }
        }

        if (isPromptLine) {
            continue;
        }

        // 跳过echo命令的结束标记相关输出
        if (line.find("echo \"<<<COMMAND_END>>>\"") != std::string::npos ||
            line.find("<<<COMMAND_END>>>") != std::string::npos) {
            continue;
        }

        // 跳过不完整的命令行（以echo "开头但没有闭合引号的行）
        if (line.find("echo \"") != std::string::npos &&
            line.rfind("\"") == line.find("echo \"") + 5) {
            // 这是一个不完整的echo命令（只有开头的引号）
            continue;
        }

        // 跳过看起来像是提示符延续的行（比如 [echo "] > "）
        if (std::regex_match(line, std::regex(R"(^[^\]]*\] > $)")) ||
            std::regex_match(line, std::regex(R"(^.*\[.*\] > $)"))) {
            continue;
        }

        lines.push_back(line);
    }

    // 重新组合清理后的行
    cleaned.clear();
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) cleaned += "\n";
        cleaned += lines[i];
    }

    // 移除开头和结尾的空白字符
    cleaned = std::regex_replace(cleaned, std::regex(R"(^\s+|\s+$)"), "");

    // 限制输出长度
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

// 获取当前工作目录
std::string getCurrentWorkingDirectory() {
    char buffer[PATH_MAX];
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
        return std::string(buffer);
    }
    return "";
}

// 从bash会话获取当前工作目录
std::string getCurrentWorkingDirectoryFromSession(
    const std::string& session_id) {
    std::lock_guard<std::mutex> lock(g_sessionsMutex);

    auto it = g_sessions.find(session_id);
    if (it == g_sessions.end()) {
        return getCurrentWorkingDirectory();  // 会话不存在，回退到进程当前目录
    }

    TerminalSession& session = it->second;

    // 向会话发送pwd命令获取当前目录
    std::string pwdCommand = "pwd\necho \"<<<PWD_END>>>\"\n";

    ssize_t written =
        write(session.inputFd, pwdCommand.c_str(), pwdCommand.length());
    if (written == -1) {
        return getCurrentWorkingDirectory();  // 回退到进程当前目录
    }

    // 读取pwd命令的输出
    std::string output;
    char buffer[4096];
    ssize_t bytesRead;
    bool foundEnd = false;

    while (!foundEnd) {
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

                if (output.find("<<<PWD_END>>>") != std::string::npos) {
                    foundEnd = true;
                    size_t endPos = output.find("<<<PWD_END>>>");
                    if (endPos != std::string::npos) {
                        output = output.substr(0, endPos);
                    }
                }
            }
        }
    }

    // 清理输出并提取目录路径
    std::string cleanedOutput = cleanOutput(output, "pwd");

    // 查找最后一行非空内容作为当前目录
    std::istringstream iss(cleanedOutput);
    std::string line, lastLine;
    while (std::getline(iss, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (!line.empty()) {
            lastLine = line;
        }
    }

    return lastLine.empty() ? getCurrentWorkingDirectory() : lastLine;
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
            close(outputPipe[1]);  // 关闭写端            // 保存会话信息
            TerminalSession session;
            session.pid = pid;
            session.inputFd = inputPipe[1];
            session.outputFd = outputPipe[0];
            session.lastUsed = std::chrono::steady_clock::now();

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
                                 command, session_id));

        // 清理超时会话
        cleanupTimeoutSessions();

        std::unique_lock<std::mutex> lock(g_sessionsMutex);

        // 查找现有会话
        auto it = g_sessions.find(session_id);
        if (it == g_sessions.end()) {
            // 如果会话不存在，自动创建一个新会话
            Logger::info(std::format(
                "Session {} not found, creating new session", session_id));

            // 释放锁来创建会话
            lock.unlock();

            // 创建新会话
            auto createResult = createTerminalSession(session_id);
            if (!createResult["success"]) {
                result["success"] = false;
                result["error"] = "Failed to create terminal session";
                result["command"] = command;
                result["session_id"] = session_id;
                result["finished"] = true;
                result["timestamp"] =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
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

        // 现在获取 session 的副本以避免并发访问问题
        pid_t sessionPid = it->second.pid;
        int inputFd = it->second.inputFd;
        int outputFd = it->second.outputFd;

        // 更新最后使用时间
        it->second.lastUsed = std::chrono::steady_clock::now();

        // 现在可以安全地释放锁
        lock.unlock();

        // 检查进程是否还活着
        int status;
        pid_t check_result = waitpid(sessionPid, &status, WNOHANG);
        if (check_result != 0) {
            // 进程已死，移除会话并返回错误
            Logger::info(std::format(
                "Session {} process is dead, removing session", session_id));

            // 需要重新获取锁来清理死掉的会话
            {
                std::lock_guard<std::mutex> cleanup_lock(g_sessionsMutex);
                auto cleanup_it = g_sessions.find(session_id);
                if (cleanup_it != g_sessions.end()) {
                    if (cleanup_it->second.inputFd != -1)
                        close(cleanup_it->second.inputFd);
                    if (cleanup_it->second.outputFd != -1)
                        close(cleanup_it->second.outputFd);
                    g_sessions.erase(cleanup_it);
                }
            }

            result["success"] = false;
            result["error"] = "Terminal session process is dead";
            result["command"] = command;
            result["session_id"] = session_id;
            result["finished"] = true;
            result["timestamp"] =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            return result;
        }

        // 向终端写入命令
        // 使用一个更隐蔽的结束标记，减少干扰
        std::string commandWithNewline =
            command + "\necho \"<<<COMMAND_END>>>\"\n";
        ssize_t written = write(inputFd, commandWithNewline.c_str(),
                                commandWithNewline.length());
        if (written == -1) {
            result["success"] = false;
            result["error"] = "Failed to write command to terminal";
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
        int consecutiveTimeouts = 0;
        const int MAX_TIMEOUTS = 30;  // 最多等待3秒 (30 * 100ms)

        while (!foundEnd && consecutiveTimeouts < MAX_TIMEOUTS) {
            // 使用select实现超时读取
            fd_set readfds;
            struct timeval tv;
            FD_ZERO(&readfds);
            FD_SET(outputFd, &readfds);
            tv.tv_sec = 0;
            tv.tv_usec = 100000;  // 100ms

            int selectResult =
                select(outputFd + 1, &readfds, nullptr, nullptr, &tv);

            if (selectResult > 0 && FD_ISSET(outputFd, &readfds)) {
                consecutiveTimeouts = 0;  // 重置超时计数
                bytesRead = read(outputFd, buffer, sizeof(buffer) - 1);
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
                } else if (bytesRead == 0) {
                    // EOF - 管道关闭
                    break;
                }
            } else if (selectResult == 0) {
                // 超时
                consecutiveTimeouts++;
            } else {
                // 错误
                break;
            }
        }

        // 构建结果
        result["success"] = true;
        result["command"] = command;
        result["session_id"] = session_id;
        result["stdout"] = cleanOutput(output, command);
        result["stderr"] = "";
        result["exit_code"] = 0;
        result["finished"] = true;

        result["cwd"] = getCurrentWorkingDirectoryFromSession(session_id);

        result["timestamp"] =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

        Logger::info(
            std::format("Command \"{}\" executed successfully in session: {}",
                        command, session_id));

    } catch (const std::exception& e) {
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

nlohmann::json forceKillSession(const std::string& session_id) {
    nlohmann::json result;
    std::lock_guard<std::mutex> lock(g_sessionsMutex);

    auto it = g_sessions.find(session_id);
    if (it == g_sessions.end()) {
        result["success"] = false;
        result["error"] = "Session not found";
        result["session_id"] = session_id;
        return result;
    }
    try {
        TerminalSession& session = it->second;

        // 强制终止进程
        if (session.pid > 0) {
            if (kill(session.pid, SIGKILL) == 0) {
                Logger::info(std::format("Force killed session process: {}",
                                         session_id));
                // 等待子进程结束，避免僵尸进程
                waitpid(session.pid, nullptr, WNOHANG);
            }
        }

        // 关闭文件描述符
        if (session.inputFd >= 0) close(session.inputFd);
        if (session.outputFd >= 0) close(session.outputFd);

        // 从会话列表中移除
        g_sessions.erase(it);

        result["success"] = true;
        result["message"] = "Session process forcefully terminated and removed";
        result["session_id"] = session_id;

        Logger::info(std::format("Removed session: {}", session_id));

    } catch (const std::exception& e) {
        result["success"] = false;
        result["error"] = e.what();
        result["session_id"] = session_id;
    }

    return result;
}

}  // namespace TerminalManager