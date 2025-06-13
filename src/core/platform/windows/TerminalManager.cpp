#define _CRT_SECURE_NO_WARNINGS

#include "core/TerminalManager.h"

#include <windows.h>

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
    HANDLE hProcess;
    HANDLE hInputWrite;
    HANDLE hOutputRead;
    std::chrono::steady_clock::time_point lastUsed;
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

// 获取当前工作目录
std::string getCurrentWorkingDirectory() {
    char buffer[MAX_PATH];
    DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (length == 0 || length > MAX_PATH) {
        return "";
    }
    return std::string(buffer, length);
}

// 从PowerShell会话获取当前工作目录
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
    DWORD bytesWritten;

    if (!WriteFile(session.hInputWrite, pwdCommand.c_str(),
                   static_cast<DWORD>(pwdCommand.length()), &bytesWritten,
                   nullptr)) {
        return getCurrentWorkingDirectory();  // 回退到进程当前目录
    }

    // 读取pwd命令的输出
    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    bool foundEnd = false;

    while (!foundEnd) {
        if (ReadFile(session.hOutputRead, buffer, sizeof(buffer) - 1,
                     &bytesRead, nullptr) &&
            bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;

            if (output.find("<<<PWD_END>>>") != std::string::npos) {
                foundEnd = true;
                size_t endPos = output.find("<<<PWD_END>>>");
                if (endPos != std::string::npos) {
                    output = output.substr(0, endPos);
                }
            }
        } else {
            Sleep(10);
        }
    }

    // 清理输出并提取目录路径
    std::string cleanedOutput = cleanOutput(output);

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
        }  // 获取 PowerShell 的完整路径
        std::string pwshPath;
        char pathBuffer[MAX_PATH];

        // 首先尝试在 PATH 中查找 pwsh.exe
        DWORD pathLen = SearchPathA(nullptr, "pwsh.exe", nullptr, MAX_PATH,
                                    pathBuffer, nullptr);
        if (pathLen > 0 && pathLen < MAX_PATH) {
            pwshPath = pathBuffer;
        } else {
            // 如果在 PATH 中找不到，尝试常见的安装位置
            std::vector<std::string> commonPaths = {
                "C:\\Program Files\\PowerShell\\7\\pwsh.exe",
                "C:\\Program Files (x86)\\PowerShell\\7\\pwsh.exe",
                "C:\\Users\\" +
                    std::string(getenv("USERNAME") ? getenv("USERNAME")
                                                   : "Public") +
                    "\\AppData\\Local\\Microsoft\\WindowsApps\\pwsh.exe"};

            for (const auto& path : commonPaths) {
                DWORD attrs = GetFileAttributesA(path.c_str());
                if (attrs != INVALID_FILE_ATTRIBUTES &&
                    !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                    pwshPath = path;
                    break;
                }
            }

            // 如果还是找不到，尝试 Windows PowerShell
            if (pwshPath.empty()) {
                pwshPath =
                    "C:\\Windows\\System32\\WindowsPowerShell\\v1."
                    "0\\powershell.exe";
            }
        }

        // 获取用户的主目录作为工作目录
        std::string workingDir;
        char* userProfile = getenv("USERPROFILE");
        if (userProfile) {
            workingDir = userProfile;
        } else {
            workingDir = "C:\\";
        }

        // 设置启动信息
        STARTUPINFOA si = {};
        si.cb = sizeof(STARTUPINFOA);
        si.hStdError = hOutputWrite;
        si.hStdOutput = hOutputWrite;
        si.hStdInput = hInputRead;
        si.dwFlags |= STARTF_USESTDHANDLES;

        PROCESS_INFORMATION pi = {};

        // 构建命令行，使用完整路径
        std::string cmdLine;
        if (pwshPath.find("powershell.exe") != std::string::npos) {
            // Windows PowerShell
            cmdLine = "\"" + pwshPath + "\" -NoExit -Command -";
        } else {
            // PowerShell Core (pwsh)
            cmdLine = "\"" + pwshPath + "\" -NoExit -Command -";
        }

        Logger::info(
            std::format("Starting PowerShell with command: {}", cmdLine));
        Logger::info(std::format("Working directory: {}", workingDir));
        if (!CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()),
                            nullptr, nullptr, TRUE,
                            CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                            nullptr, workingDir.c_str(), &si, &pi)) {
            DWORD error = GetLastError();
            CloseHandle(hInputRead);
            CloseHandle(hInputWrite);
            CloseHandle(hOutputRead);
            CloseHandle(hOutputWrite);
            result["success"] = false;
            result["error"] =
                std::format("Failed to create process. Error: {}", error);
            Logger::error(
                std::format("CreateProcess failed with error: {}", error));
            return result;
        }  // 关闭不需要的句柄
        CloseHandle(hInputRead);
        CloseHandle(hOutputWrite);
        CloseHandle(pi.hThread);

        // 保存会话信息
        TerminalSession session;
        session.hProcess = pi.hProcess;
        session.hInputWrite = hInputWrite;
        session.hOutputRead = hOutputRead;
        session.lastUsed = std::chrono::steady_clock::now();

        g_sessions[session_id] = session;

        // 等待 PowerShell 启动完成，然后发送初始化命令
        Sleep(1000);  // 等待1秒让PowerShell完全启动

        // 发送初始化命令设置执行策略和工作目录
        std::string initCommands =
            "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process "
            "-Force\n"
            "cd \"" +
            workingDir +
            "\"\n"
            "Clear-Host\n"
            "echo \"<<<INIT_COMPLETE>>>\"\n";

        DWORD bytesWritten;
        if (WriteFile(hInputWrite, initCommands.c_str(),
                      static_cast<DWORD>(initCommands.length()), &bytesWritten,
                      nullptr)) {
            // 等待初始化完成
            std::string output;
            char buffer[1024];
            DWORD bytesRead;
            bool initComplete = false;

            auto startTime = std::chrono::steady_clock::now();
            while (!initComplete &&
                   std::chrono::steady_clock::now() - startTime <
                       std::chrono::seconds(5)) {
                DWORD bytesAvailable = 0;
                if (PeekNamedPipe(hOutputRead, nullptr, 0, nullptr,
                                  &bytesAvailable, nullptr) &&
                    bytesAvailable > 0) {
                    if (ReadFile(hOutputRead, buffer, sizeof(buffer) - 1,
                                 &bytesRead, nullptr) &&
                        bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        output += buffer;
                        if (output.find("<<<INIT_COMPLETE>>>") !=
                            std::string::npos) {
                            initComplete = true;
                        }
                    }
                } else {
                    Sleep(10);
                }
            }

            Logger::info(std::format(
                "PowerShell initialization completed for session: {}",
                session_id));
        }

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
                                 command, session_id));  // 清理超时会话
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
        }  // 现在获取 session 的副本以避免并发访问问题
        HANDLE hProcess = it->second.hProcess;
        HANDLE hInputWrite = it->second.hInputWrite;
        HANDLE hOutputRead = it->second.hOutputRead;  // 更新最后使用时间
        it->second.lastUsed = std::chrono::steady_clock::now();

        // 现在可以安全地释放锁
        lock.unlock();

        // 检查进程是否还活着
        DWORD exitCode;
        if (!GetExitCodeProcess(hProcess, &exitCode) ||
            exitCode != STILL_ACTIVE) {  // 进程已死，移除会话并返回错误
            Logger::info(std::format(
                "Session {} process is dead, removing session", session_id));

            // 需要重新获取锁来清理死掉的会话
            {
                std::lock_guard<std::mutex> cleanup_lock(g_sessionsMutex);
                auto cleanup_it = g_sessions.find(session_id);
                if (cleanup_it != g_sessions.end()) {
                    if (cleanup_it->second.hInputWrite)
                        CloseHandle(cleanup_it->second.hInputWrite);
                    if (cleanup_it->second.hOutputRead)
                        CloseHandle(cleanup_it->second.hOutputRead);
                    if (cleanup_it->second.hProcess)
                        CloseHandle(cleanup_it->second.hProcess);
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
        std::string commandWithNewline =
            command + "\necho \"<<<COMMAND_END>>>\"\n";
        DWORD bytesWritten;
        if (!WriteFile(hInputWrite, commandWithNewline.c_str(),
                       static_cast<DWORD>(commandWithNewline.length()),
                       &bytesWritten, nullptr)) {
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
        }  // 读取输出直到看到结束标记
        std::string output;
        char buffer[4096];
        DWORD bytesRead;
        bool foundEnd = false;
        while (!foundEnd) {
            // 使用非阻塞方式检查是否有数据可读
            DWORD bytesAvailable = 0;
            if (PeekNamedPipe(hOutputRead, nullptr, 0, nullptr, &bytesAvailable,
                              nullptr)) {
                if (bytesAvailable > 0) {
                    // 有数据可读，执行ReadFile
                    if (ReadFile(hOutputRead, buffer, sizeof(buffer) - 1,
                                 &bytesRead, nullptr) &&
                        bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        output += buffer;

                        // 检查是否找到结束标记
                        if (output.find("<<<COMMAND_END>>>") !=
                            std::string::npos) {
                            foundEnd = true;
                            // 移除结束标记及其后的内容
                            size_t endPos = output.find("<<<COMMAND_END>>>");
                            if (endPos != std::string::npos) {
                                output = output.substr(0, endPos);
                            }
                        }
                    }
                } else {
                    // 没有数据可读，短暂等待后再次检查中断标志
                    Sleep(10);
                }
            } else {
                // PeekNamedPipe失败，短暂等待
                Sleep(10);
            }
        }

        // 构建结果
        result["success"] = true;
        result["command"] = command;
        result["session_id"] = session_id;
        result["stdout"] = cleanOutput(output);
        result["stderr"] = "";
        result["exit_code"] = 0;
        result["finished"] = true;  // 获取当前工作目录
        std::string cwd = getCurrentWorkingDirectoryFromSession(session_id);
        result["cwd"] = cwd;

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
        if (session.hProcess) {
            if (TerminateProcess(session.hProcess, 1)) {
                Logger::info(std::format("Force killed session process: {}",
                                         session_id));
            }
            CloseHandle(session.hProcess);
        }

        // 关闭其他句柄
        if (session.hInputWrite) CloseHandle(session.hInputWrite);
        if (session.hOutputRead) CloseHandle(session.hOutputRead);

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