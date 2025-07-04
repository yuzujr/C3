#ifndef PTY_MANAGER_H
#define PTY_MANAGER_H

#include <functional>
#include <memory>
#include <string>

#include "nlohmann/json.hpp"

#if defined(_WIN32)
#define SHELL_EXECUTABLE "cmd.exe"
#elif defined(__linux__)
#define SHELL_EXECUTABLE "bash"
#endif

// ===========================================
// PTY 管理器类
// ===========================================

class PtyManager {
public:
    // 获取单例实例
    static PtyManager& getInstance();

    // 禁止拷贝和赋值
    PtyManager(const PtyManager&) = delete;
    PtyManager& operator=(const PtyManager&) = delete;

    // 输出回调函数类型：当PTY有输出时调用
    // 参数: {
    //   type: 输出类型 (如 "shell_output", "kill_session_result")
    //   session_id: 会话ID
    //   data: 输出数据
    // }
    using OutputCallback = std::function<void(const nlohmann::json&)>;

    // 设置输出回调函数
    void setOutputCallback(OutputCallback callback);

    // 创建一个新的PTY会话
    nlohmann::json createPtySession(
        const std::string& session_id, int cols = 80, int rows = 24,
        const std::string& command = SHELL_EXECUTABLE);

    // 向PTY会话写入数据（如果会话不存在会自动创建）
    void writeToPtySession(const std::string& session_id,
                           const std::string& data);

    // 调整PTY会话大小
    nlohmann::json resizePtySession(const std::string& session_id, int cols,
                                    int rows);

    // 关闭PTY会话
    nlohmann::json closePtySession(const std::string& session_id);

    // 关闭所有活跃的PTY会话（应用程序退出时调用）
    void shutdownAllPtySessions();

    // 重置PTY管理器状态（测试时使用）
    void reset();

private:
    PtyManager();
    ~PtyManager();

    // PIMPL 模式隐藏实现细节
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif  // PTY_MANAGER_H
