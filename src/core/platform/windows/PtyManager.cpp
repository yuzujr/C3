#include "core/PtyManager.h"

#include <Windows.h>
#include <process.h>

#include <chrono>
#include <future>
#include <map>
#include <mutex>
#include <thread>

namespace PtyManager {

// ===========================================
// 内部数据结构和状态
// ===========================================

// PTY会话信息
struct PtySession {
    HPCON hPseudoConsole;
    HANDLE hProcess;
    HANDLE hPipeIn;   // 写入PTY的管道
    HANDLE hPipeOut;  // 从PTY读取的管道
    std::thread outputThread;
    std::chrono::steady_clock::time_point lastUsed;
};

static std::map<std::string, PtySession> g_pty_sessions;
static std::mutex g_pty_sessionsMutex;
static OutputCallback g_outputCallback;
static const auto SESSION_TIMEOUT = std::chrono::minutes(30);
static bool g_shutdownCalled = false;

// ===========================================
// 内部辅助函数
// ===========================================

// 统一的 JSON 响应格式
static nlohmann::json createResponse(
    bool success, const std::string& message = "",
    const std::string& session_id = "",
    const nlohmann::json& data = nlohmann::json::object()) {
    nlohmann::json response;
    response["success"] = success;
    response["message"] = message;
    if (!session_id.empty()) {
        response["session_id"] = session_id;
    }
    if (!data.empty()) {
        response["data"] = data;
    }
    return response;
}

// 发送输出到服务端
static void sendOutput(const std::string& type, const std::string& session_id,
                       const nlohmann::json& data) {
    if (g_outputCallback) {
        nlohmann::json response = {
            {"type", type}, {"session_id", session_id}, {"data", data}};
        g_outputCallback(response);
    }
}

// 内部函数声明
static void cleanupTimeoutPtySessions();
static HRESULT CreatePseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn,
                                           HANDLE* phPipeOut, int cols,
                                           int rows);
static HRESULT InitializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEXW*,
                                                            HPCON);
static void PtyOutputListener(const std::string& session_id, HANDLE hPipeOut);

// ===========================================
// 外部接口实现
// ===========================================

nlohmann::json createPtySession(const std::string& session_id, int cols,
                                int rows, const std::string& command) {
    cleanupTimeoutPtySessions();

    std::lock_guard<std::mutex> lock(g_pty_sessionsMutex);
    if (g_pty_sessions.count(session_id)) {
        return createResponse(false, "Session already exists", session_id);
    }

    HPCON hPC{INVALID_HANDLE_VALUE};
    HANDLE hPipeIn{INVALID_HANDLE_VALUE};
    HANDLE hPipeOut{INVALID_HANDLE_VALUE};

    HRESULT hr =
        CreatePseudoConsoleAndPipes(&hPC, &hPipeIn, &hPipeOut, cols, rows);
    if (FAILED(hr)) {
        return createResponse(false, "Failed to create pseudo console",
                              session_id);
    }

    STARTUPINFOEXW startupInfo{};
    if (FAILED(
            InitializeStartupInfoAttachedToPseudoConsole(&startupInfo, hPC))) {
        ClosePseudoConsole(hPC);
        CloseHandle(hPipeIn);
        CloseHandle(hPipeOut);
        return createResponse(false, "Failed to initialize startup info",
                              session_id);
    }

    PROCESS_INFORMATION piClient{};
    std::wstring wcommand(command.begin(), command.end());
    hr = CreateProcessW(NULL, &wcommand[0], NULL, NULL, FALSE,
                        EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
                        &startupInfo.StartupInfo, &piClient)
             ? S_OK
             : GetLastError();

    DeleteProcThreadAttributeList(startupInfo.lpAttributeList);
    free(startupInfo.lpAttributeList);

    if (FAILED(hr)) {
        ClosePseudoConsole(hPC);
        CloseHandle(hPipeIn);
        CloseHandle(hPipeOut);
        return createResponse(false, "Failed to create process", session_id);
    }

    PtySession session;
    session.hPseudoConsole = hPC;
    session.hProcess = piClient.hProcess;
    session.hPipeIn = hPipeIn;
    session.hPipeOut = hPipeOut;
    session.lastUsed = std::chrono::steady_clock::now();
    session.outputThread =
        std::thread(PtyOutputListener, session_id, session.hPipeIn);

    g_pty_sessions[session_id] = std::move(session);
    CloseHandle(piClient.hThread);

    return createResponse(true, "PTY session created", session_id);
}

void writeToPtySession(const std::string& session_id, const std::string& data) {
    bool session_exists = false;
    {
        std::lock_guard<std::mutex> lock(g_pty_sessionsMutex);
        session_exists =
            (g_pty_sessions.find(session_id) != g_pty_sessions.end());
    }

    // 如果会话不存在，创建新会话
    if (!session_exists) {
        auto create_result = createPtySession(session_id);
        if (!create_result["success"].get<bool>()) {
            sendOutput("shell_output", session_id,
                       {{"success", false},
                        {"error", create_result["message"]},
                        {"output", ""}});
            return;
        }
    }

    nlohmann::json result;
    {
        std::lock_guard<std::mutex> lock(g_pty_sessionsMutex);
        auto it = g_pty_sessions.find(session_id);
        if (it != g_pty_sessions.end()) {
            it->second.lastUsed = std::chrono::steady_clock::now();
            DWORD bytesWritten;
            if (WriteFile(it->second.hPipeOut, data.c_str(),
                          (DWORD)data.length(), &bytesWritten, NULL)) {
                result = {{"success", true}, {"output", ""}};
            } else {
                result = {{"success", false},
                          {"error", "Failed to write to pipe"},
                          {"output", ""}};
            }
        } else {
            result = {{"success", false},
                      {"error", "Session not found"},
                      {"output", ""}};
        }
    }

    sendOutput("shell_output", session_id, result);
}

nlohmann::json resizePtySession(const std::string& session_id, int cols,
                                int rows) {
    std::lock_guard<std::mutex> lock(g_pty_sessionsMutex);
    auto it = g_pty_sessions.find(session_id);
    if (it == g_pty_sessions.end()) {
        return createResponse(false, "Session not found", session_id);
    }

    it->second.lastUsed = std::chrono::steady_clock::now();
    HRESULT hr = ResizePseudoConsole(it->second.hPseudoConsole,
                                     {(SHORT)cols, (SHORT)rows});

    return FAILED(hr)
               ? createResponse(false, "Failed to resize PTY", session_id)
               : createResponse(true, "PTY resized", session_id);
}

nlohmann::json closePtySession(const std::string& session_id) {
    std::unique_lock<std::mutex> lock(g_pty_sessionsMutex);
    auto it = g_pty_sessions.find(session_id);
    if (it == g_pty_sessions.end()) {
        return createResponse(false, "Session not found", session_id);
    }

    PtySession session = std::move(it->second);
    g_pty_sessions.erase(it);
    lock.unlock();

    // 清理资源
    if (session.hProcess) {
        TerminateProcess(session.hProcess, 0);
        WaitForSingleObject(session.hProcess, 1000);
        CloseHandle(session.hProcess);
    }

    if (session.hPseudoConsole) {
        ClosePseudoConsole(session.hPseudoConsole);
    }

    if (session.hPipeIn) CloseHandle(session.hPipeIn);
    if (session.hPipeOut) CloseHandle(session.hPipeOut);

    // 等待输出线程结束
    if (session.outputThread.joinable()) {
        auto future = std::async(std::launch::async, [&]() {
            session.outputThread.join();
        });

        if (future.wait_for(std::chrono::seconds(2)) ==
            std::future_status::timeout) {
            session.outputThread.detach();
        } else {
            future.get();
        }
    }

    auto result = createResponse(true, "Session closed", session_id);

    // 发送关闭结果到服务端
    sendOutput("kill_session_result", session_id, result);

    return result;
}

void setOutputCallback(OutputCallback callback) {
    g_outputCallback = callback;
}

void shutdownAllPtySessions() {
    {
        std::lock_guard<std::mutex> lock(g_pty_sessionsMutex);
        if (g_shutdownCalled) return;
        g_shutdownCalled = true;
    }

    std::vector<std::string> sessionsToClose;
    {
        std::lock_guard<std::mutex> lock(g_pty_sessionsMutex);
        for (const auto& [session_id, session] : g_pty_sessions) {
            sessionsToClose.push_back(session_id);
        }
    }

    for (const auto& session_id : sessionsToClose) {
        closePtySession(session_id);
    }

    g_outputCallback = nullptr;
}

// 重置PTY管理器状态（测试时使用）
void resetPtyManager() {
    std::lock_guard<std::mutex> lock(g_pty_sessionsMutex);
    g_shutdownCalled = false;
    g_outputCallback = nullptr;
}

// ===========================================
// 内部函数实现
// ===========================================

static void PtyOutputListener(const std::string& session_id, HANDLE hPipeOut) {
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hPipeOut, buffer, sizeof(buffer), &bytesRead, NULL) &&
           bytesRead > 0) {
        sendOutput(
            "shell_output", session_id,
            {{"success", true}, {"output", std::string(buffer, bytesRead)}});
    }
}

static HRESULT InitializeStartupInfoAttachedToPseudoConsole(
    STARTUPINFOEXW* pStartupInfo, HPCON hPC) {
    HRESULT hr{E_UNEXPECTED};

    if (pStartupInfo) {
        size_t attrListSize{};

        pStartupInfo->StartupInfo.cb = sizeof(STARTUPINFOEXW);

        InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);

        pStartupInfo->lpAttributeList =
            reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
                malloc(attrListSize));

        if (pStartupInfo->lpAttributeList &&
            InitializeProcThreadAttributeList(pStartupInfo->lpAttributeList, 1,
                                              0, &attrListSize)) {
            hr = UpdateProcThreadAttribute(pStartupInfo->lpAttributeList, 0,
                                           PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                           hPC, sizeof(HPCON), NULL, NULL)
                     ? S_OK
                     : HRESULT_FROM_WIN32(GetLastError());
        } else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}

static void cleanupTimeoutPtySessions() {
    std::vector<std::string> sessionsToClose;
    {
        std::lock_guard<std::mutex> lock(g_pty_sessionsMutex);
        auto now = std::chrono::steady_clock::now();
        for (const auto& [id, session] : g_pty_sessions) {
            if (now - session.lastUsed > SESSION_TIMEOUT) {
                sessionsToClose.push_back(id);
            }
        }
    }

    for (const auto& id : sessionsToClose) {
        closePtySession(id);
    }
}

// Create the Pseudo Console and pipes to it
static HRESULT CreatePseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn,
                                           HANDLE* phPipeOut, int cols,
                                           int rows) {
    HRESULT hr{E_UNEXPECTED};
    HANDLE hPipePTYIn{INVALID_HANDLE_VALUE};
    HANDLE hPipePTYOut{INVALID_HANDLE_VALUE};

    // Create the pipes to which the ConPTY will connect
    if (CreatePipe(&hPipePTYIn, phPipeOut, NULL, 0) &&
        CreatePipe(phPipeIn, &hPipePTYOut, NULL, 0)) {
        // Use provided console size
        COORD consoleSize{(SHORT)cols, (SHORT)rows};

        // Create the Pseudo Console of the required size, attached to the
        // PTY-end of the pipes with VT processing enabled
        hr = CreatePseudoConsole(consoleSize, hPipePTYIn, hPipePTYOut, 0, phPC);

        // Note: We can close the handles to the PTY-end of the pipes here
        // because the handles are dup'ed into the ConHost and will be released
        // when the ConPTY is destroyed.
        if (INVALID_HANDLE_VALUE != hPipePTYOut) CloseHandle(hPipePTYOut);
        if (INVALID_HANDLE_VALUE != hPipePTYIn) CloseHandle(hPipePTYIn);
    }

    return hr;
}

}  // namespace PtyManager
