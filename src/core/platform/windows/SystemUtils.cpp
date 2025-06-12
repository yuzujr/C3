#include "core/SystemUtils.h"

#include <shellscalingapi.h>
#include <windows.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <map>
#include <mutex>
#include <regex>

#include "core/Logger.h"

namespace SystemUtils {

// 终端会话管理
struct TerminalSession {
    HANDLE hProcess;
    HANDLE hInputWrite;
    HANDLE hOutputRead;
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

    // 移除多余的空行 (连续的\r\n)
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

void enableHighDPI() {
    // 启用高 DPI 感知
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}

void addToStartup(const std::string& appName) {
    std::string exePath = getExecutablePath();
    HKEY hKey = nullptr;
    const char* runKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, runKey, 0, KEY_SET_VALUE, &hKey) ==
        ERROR_SUCCESS) {
        RegSetValueExA(hKey, appName.c_str(), 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(exePath.c_str()),
                       static_cast<DWORD>(exePath.size() + 1));
        RegCloseKey(hKey);
    }
}

void removeFromStartup(const std::string& appName) {
    HKEY hKey = nullptr;
    const char* runKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, runKey, 0, KEY_SET_VALUE, &hKey) ==
        ERROR_SUCCESS) {
        RegDeleteValueA(hKey, appName.c_str());
        RegCloseKey(hKey);
    }
}

std::string getExecutablePath() {
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (length == 0 || length == MAX_PATH) return "";  // 失败或路径太长
    return std::string(path, length);
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
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE hInputRead, hInputWrite;
        HANDLE hOutputRead, hOutputWrite;

        if (!CreatePipe(&hInputRead, &hInputWrite, &sa, 0) ||
            !CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0)) {
            result["success"] = false;
            result["error"] = "Failed to create pipes";
            return result;
        }

        // 设置启动信息
        STARTUPINFOA si = {};
        si.cb = sizeof(STARTUPINFOA);
        si.hStdError = hOutputWrite;
        si.hStdOutput = hOutputWrite;
        si.hStdInput = hInputRead;
        si.dwFlags |= STARTF_USESTDHANDLES;

        PROCESS_INFORMATION pi = {};

        // 启动PowerShell进程
        std::string cmdLine = "pwsh.exe -NoExit -Command -";
        if (!CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()),
                            nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si,
                            &pi)) {
            CloseHandle(hInputRead);
            CloseHandle(hInputWrite);
            CloseHandle(hOutputRead);
            CloseHandle(hOutputWrite);
            result["success"] = false;
            result["error"] = "Failed to create process";
            return result;
        }

        // 关闭不需要的句柄
        CloseHandle(hInputRead);
        CloseHandle(hOutputWrite);
        CloseHandle(pi.hThread);

        // 保存会话信息
        TerminalSession session;
        session.hProcess = pi.hProcess;
        session.hInputWrite = hInputWrite;
        session.hOutputRead = hOutputRead;
        session.lastUsed = std::chrono::steady_clock::now();
        session.isActive = true;

        g_sessions[session_id] = session;

        result["success"] = true;
        result["message"] = "Terminal session created successfully";
        result["session_id"] = session_id;

        Logger::info(std::format("Created terminal session: {}", session_id));

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

        // 关闭句柄
        if (session.hInputWrite) CloseHandle(session.hInputWrite);
        if (session.hOutputRead) CloseHandle(session.hOutputRead);
        if (session.hProcess) {
            TerminateProcess(session.hProcess, 0);
            CloseHandle(session.hProcess);
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

            // 关闭句柄
            if (it->second.hInputWrite) CloseHandle(it->second.hInputWrite);
            if (it->second.hOutputRead) CloseHandle(it->second.hOutputRead);
            if (it->second.hProcess) {
                TerminateProcess(it->second.hProcess, 0);
                CloseHandle(it->second.hProcess);
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

        std::lock_guard<std::mutex> lock(g_sessionsMutex);  // 查找现有会话
        auto it = g_sessions.find(session_id);
        if (it == g_sessions.end()) {
            // 如果会话不存在，自动创建一个新会话
            Logger::info(std::format(
                "Session {} not found, creating new session", session_id));

            // 释放锁来创建会话
            g_sessionsMutex.unlock();

            // 创建新会话
            auto createResult = createTerminalSession(session_id);
            if (!createResult["success"]) {
                Logger::error(
                    std::format("Failed to create session {}: {}", session_id,
                                createResult["error"].get<std::string>()));

                // 如果创建会话失败，回退到一次性执行
                std::string full_command =
                    std::format("pwsh.exe -Command \"{}\"", command);
                FILE* pipe = _popen(full_command.c_str(), "r");
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

                int exit_code = _pclose(pipe);
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
            g_sessionsMutex.lock();
            it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                result["success"] = false;
                result["error"] = "Failed to find newly created session";
                return result;
            }
        }

        TerminalSession& session = it->second;

        // 检查进程是否还活着
        DWORD exitCode;
        if (!GetExitCodeProcess(session.hProcess, &exitCode) ||
            exitCode != STILL_ACTIVE) {
            // 进程已死，移除会话并回退到一次性执行
            Logger::info(std::format(
                "Session {} process is dead, removing session", session_id));

            // 清理死掉的会话
            if (session.hInputWrite) CloseHandle(session.hInputWrite);
            if (session.hOutputRead) CloseHandle(session.hOutputRead);
            if (session.hProcess) CloseHandle(session.hProcess);
            g_sessions.erase(it);

            // 释放锁并回退到一次性执行
            g_sessionsMutex.unlock();

            std::string full_command =
                std::format("pwsh.exe -Command \"{}\"", command);
            FILE* pipe = _popen(full_command.c_str(), "r");
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

            int exit_code = _pclose(pipe);
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

        // 向终端写入命令 (不需要释放锁，因为我们在操作会话)
        std::string commandWithNewline =
            command + "\necho \"<<<COMMAND_END>>>\"\n";
        DWORD bytesWritten;
        if (!WriteFile(session.hInputWrite, commandWithNewline.c_str(),
                       static_cast<DWORD>(commandWithNewline.length()),
                       &bytesWritten, nullptr)) {
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
        DWORD bytesRead;
        bool foundEnd = false;
        auto startTime = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(30);  // 30秒超时
        while (!foundEnd) {
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
                Logger::info(
                    std::format("Output length exceeded limit for session: {}",
                                session_id));
                foundEnd = true;
                output += "\n\n[输出过长，已提前终止读取]";
                break;
            }

            if (ReadFile(session.hOutputRead, buffer, sizeof(buffer) - 1,
                         &bytesRead, nullptr) &&
                bytesRead > 0) {
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
            } else {
                Sleep(10);  // 短暂等待
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
