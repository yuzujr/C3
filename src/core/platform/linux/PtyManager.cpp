#include "core/PtyManager.h"

#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <future>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "core/Logger.h"

namespace PtyManager {

// PTY会话信息结构
struct PtySession {
    int master_fd = -1;
    pid_t child_pid = -1;
    std::thread outputThread;
    std::chrono::steady_clock::time_point lastUsed;
    bool isActive = true;
};

// 全局状态
static std::map<std::string, PtySession> g_pty_sessions;
static std::mutex g_mutex;
static OutputCallback g_output_callback = nullptr;
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
    if (g_output_callback) {
        nlohmann::json response = {
            {"type", type}, {"session_id", session_id}, {"data", data}};
        g_output_callback(response);
    }
}

// 清理超时会话
static void cleanupTimeoutPtySessions() {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto now = std::chrono::steady_clock::now();

    auto it = g_pty_sessions.begin();
    while (it != g_pty_sessions.end()) {
        if (now - it->second.lastUsed > SESSION_TIMEOUT) {
            Logger::info("Cleaning up timeout PTY session: " + it->first);

            // 清理资源
            PtySession& session = it->second;
            session.isActive = false;

            if (session.child_pid > 0) {
                kill(session.child_pid, SIGTERM);
                kill(session.child_pid, SIGKILL);
            }

            if (session.master_fd >= 0) {
                close(session.master_fd);
            }

            if (session.outputThread.joinable()) {
                session.outputThread.join();
            }

            it = g_pty_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

// ===========================================
// 外部接口实现
// ===========================================

// 设置输出回调函数
void setOutputCallback(OutputCallback callback) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_output_callback = callback;
}

// 创建新的PTY会话
nlohmann::json createPtySession(const std::string& session_id, int cols,
                                int rows, const std::string& command) {
    cleanupTimeoutPtySessions();

    std::lock_guard<std::mutex> lock(g_mutex);

    // 检查会话是否已存在
    if (g_pty_sessions.count(session_id)) {
        return createResponse(false, "Session already exists", session_id);
    }

    int master_fd, slave_fd;
    pid_t pid;
    struct termios termp;
    struct winsize winp = {static_cast<unsigned short>(rows),
                           static_cast<unsigned short>(cols), 0, 0};

    // 获取当前终端属性
    if (tcgetattr(STDIN_FILENO, &termp) == -1) {
        Logger::warn("Failed to get terminal attributes, using defaults");
        memset(&termp, 0, sizeof(termp));
        cfmakeraw(&termp);
    }

    // 创建PTY对
    if (openpty(&master_fd, &slave_fd, nullptr, &termp, &winp) == -1) {
        return createResponse(
            false, "Failed to create PTY: " + std::string(strerror(errno)),
            session_id);
    }

    // 设置master为非阻塞模式，避免线程卡死
    int flags = fcntl(master_fd, F_GETFL);
    fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

    // Fork子进程
    pid = fork();
    if (pid < 0) {
        close(master_fd);
        close(slave_fd);
        return createResponse(
            false, "Failed to fork process: " + std::string(strerror(errno)),
            session_id);
    }

    if (pid == 0) {
        // 子进程 - 运行shell
        close(master_fd);

        // 创建新的会话
        if (setsid() == -1) {
            Logger::error("setsid failed");
            _exit(1);
        }

        // 设置controlling terminal
        if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) {
            Logger::error("TIOCSCTTY failed");
            _exit(1);
        }

        // 重定向标准输入输出错误到slave
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        if (slave_fd > STDERR_FILENO) {
            close(slave_fd);
        }

        // 设置环境变量
        setenv("TERM", "xterm-256color", 1);
        setenv("PS1", "\\u@\\h:\\w$ ", 1);

        // 执行shell
        execl(command.c_str(), command.c_str(), nullptr);

        // 如果exec失败，尝试bash
        execl("/bin/bash", "bash", nullptr);

        // 最后尝试sh
        execl("/bin/sh", "sh", nullptr);

        Logger::error("Failed to exec shell");
        _exit(1);
    }

    // 父进程
    close(slave_fd);

    PtySession session;
    session.master_fd = master_fd;
    session.child_pid = pid;
    session.lastUsed = std::chrono::steady_clock::now();
    session.isActive = true;

    // 创建输出读取线程
    session.outputThread = std::thread([session_id, master_fd]() {
        char buffer[4096];

        while (true) {
            // 检查全局shutdown状态和会话活跃状态
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                if (g_shutdownCalled) {
                    break;
                }
                auto it = g_pty_sessions.find(session_id);
                if (it == g_pty_sessions.end() || !it->second.isActive) {
                    break;
                }
            }

            ssize_t bytes_read = read(master_fd, buffer, sizeof(buffer) - 1);

            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';

                // 更新最后使用时间
                {
                    std::lock_guard<std::mutex> lock(g_mutex);
                    auto it = g_pty_sessions.find(session_id);
                    if (it != g_pty_sessions.end()) {
                        it->second.lastUsed = std::chrono::steady_clock::now();
                    }
                }

                // 发送输出到回调
                sendOutput("shell_output", session_id,
                           {{"success", true},
                            {"output", std::string(buffer, bytes_read)}});
            } else if (bytes_read == 0) {
                // EOF - 进程结束
                Logger::info("PTY session ended: " + session_id);
                break;
            } else {
                // 读取错误或被中断
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 非阻塞读取，暂时没有数据，稍等再试
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                } else if (errno == EINTR) {
                    // 被信号中断，继续
                    continue;
                } else {
                    // 其他错误（可能是文件描述符被关闭）
                    Logger::info("PTY read terminated: " +
                                 std::string(strerror(errno)));
                    break;
                }
            }
        }
    });

    g_pty_sessions[session_id] = std::move(session);

    return createResponse(true, "PTY session created", session_id);
}

// 向PTY会话写入数据（如果会话不存在会自动创建）
void writeToPtySession(const std::string& session_id, const std::string& data) {
    nlohmann::json result;

    // 首先检查会话是否存在，不持有锁
    bool sessionExists = false;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_pty_sessions.find(session_id);
        if (it != g_pty_sessions.end()) {
            sessionExists = true;
            it->second.lastUsed = std::chrono::steady_clock::now();
            ssize_t bytes_written =
                write(it->second.master_fd, data.data(), data.size());

            if (bytes_written == static_cast<ssize_t>(data.size())) {
                result = {{"success", true}, {"output", ""}};
            } else {
                result = {{"success", false},
                          {"error", "Failed to write to PTY: " +
                                        std::string(strerror(errno))},
                          {"output", ""}};
            }
        }
    }

    // 如果会话不存在，在锁外创建会话
    if (!sessionExists) {
        Logger::info("PTY session not found, creating new session: " +
                     session_id);
        auto createResult =
            createPtySession(session_id, 120, 40, SHELL_EXECUTABLE);

        if (createResult["success"]) {
            // 会话创建成功，重试写入
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_pty_sessions.find(session_id);
            if (it != g_pty_sessions.end()) {
                ssize_t bytes_written =
                    write(it->second.master_fd, data.data(), data.size());
                if (bytes_written == static_cast<ssize_t>(data.size())) {
                    result = {{"success", true}, {"output", ""}};
                } else {
                    result = {{"success", false},
                              {"error", "Failed to write to newly created PTY"},
                              {"output", ""}};
                }
            } else {
                result = {{"success", false},
                          {"error", "Failed to find newly created session"},
                          {"output", ""}};
            }
        } else {
            result = {{"success", false},
                      {"error", "Session not found and failed to create: " +
                                    createResult["message"].get<std::string>()},
                      {"output", ""}};
        }
    }

    sendOutput("shell_output", session_id, result);
}

// 调整PTY窗口大小
nlohmann::json resizePtySession(const std::string& session_id, int cols,
                                int rows) {
    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_pty_sessions.find(session_id);
    if (it == g_pty_sessions.end()) {
        return createResponse(false, "Session not found", session_id);
    }

    it->second.lastUsed = std::chrono::steady_clock::now();

    struct winsize ws;
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_col = static_cast<unsigned short>(cols);
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    if (ioctl(it->second.master_fd, TIOCSWINSZ, &ws) == -1) {
        return createResponse(
            false, "Failed to resize PTY: " + std::string(strerror(errno)),
            session_id);
    }

    Logger::info("PTY resized: " + session_id + " to " + std::to_string(rows) +
                 "x" + std::to_string(cols));
    return createResponse(true, "PTY resized", session_id);
}

// 关闭PTY会话
nlohmann::json closePtySession(const std::string& session_id) {
    std::unique_lock<std::mutex> lock(g_mutex);
    auto it = g_pty_sessions.find(session_id);
    if (it == g_pty_sessions.end()) {
        return createResponse(false, "Session not found", session_id);
    }

    PtySession session = std::move(it->second);
    g_pty_sessions.erase(it);
    lock.unlock();

    // 清理资源 - 首先标记为非活跃状态
    session.isActive = false;

    // 关闭文件描述符，这会让输出线程的read()调用返回错误
    if (session.master_fd >= 0) {
        close(session.master_fd);
        session.master_fd = -1;
    }

    // 终止子进程
    if (session.child_pid > 0) {
        // 首先尝试优雅关闭
        kill(session.child_pid, SIGTERM);

        // 等待100ms让进程优雅退出
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 检查进程是否已经退出
        int status;
        pid_t result = waitpid(session.child_pid, &status, WNOHANG);
        if (result == 0) {
            // 进程还在运行，强制杀死
            kill(session.child_pid, SIGKILL);
            waitpid(session.child_pid, &status, 0);  // 阻塞等待
        }
    }

    // 等待输出线程结束
    if (session.outputThread.joinable()) {
        auto future = std::async(std::launch::async, [&]() {
            session.outputThread.join();
        });

        // 等待2秒，应该足够线程检测到fd关闭并退出
        if (future.wait_for(std::chrono::seconds(2)) ==
            std::future_status::timeout) {
            Logger::warn("PTY output thread timeout, detaching: " + session_id);
            session.outputThread.detach();
        } else {
            future.get();
        }
    }

    auto result = createResponse(true, "Session closed", session_id);

    // 发送关闭结果到服务端
    sendOutput("kill_session_result", session_id, result);

    Logger::info("PTY session closed: " + session_id);
    return result;
}

// 关闭所有活跃的PTY会话（应用程序退出时调用）
void shutdownAllPtySessions() {
    // 设置全局shutdown标志，防止重复调用
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_shutdownCalled) return;
        g_shutdownCalled = true;
    }

    // 收集需要关闭的会话ID
    std::vector<std::string> sessionsToClose;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (const auto& [session_id, session] : g_pty_sessions) {
            sessionsToClose.push_back(session_id);
        }
    }

    // 逐个关闭会话
    for (const auto& session_id : sessionsToClose) {
        closePtySession(session_id);
    }

    // 清空回调
    g_output_callback = nullptr;
    Logger::info("All PTY sessions shut down");
}

// 重置PTY管理器状态（测试时使用）
void resetPtyManager() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_shutdownCalled = false;
    g_output_callback = nullptr;
}

}  // namespace PtyManager