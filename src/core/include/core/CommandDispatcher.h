#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <atomic>
#include <functional>
#include <thread>

#include "core/Config.h"
#include "core/ControlCenter.h"
#include "nlohmann/json.hpp"


class CommandDispatcher {
public:
    // 截图回调函数类型
    using ScreenshotCallback = std::function<void()>;

    // Shell 命令执行回调函数类型
    // 参数: command (命令), session_id (会话ID)
    // 返回: nlohmann::json (包含执行结果)
    using ShellExecuteCallback =
        std::function<nlohmann::json(const std::string&, const std::string&)>;
    CommandDispatcher(ControlCenter& controller, Config& config)
        : m_controller(controller), m_config(config) {}
    ~CommandDispatcher();
    CommandDispatcher(const CommandDispatcher&) = delete;
    CommandDispatcher& operator=(const CommandDispatcher&) = delete;

    // 设置截图回调函数
    void setScreenshotCallback(ScreenshotCallback callback);

    // 设置 Shell 命令执行回调函数
    void setShellExecuteCallback(ShellExecuteCallback callback);
    void dispatchCommands(const nlohmann::json& commands);

    // 设置响应回调函数 (用于向服务器发送响应)
    void setResponseCallback(
        std::function<void(const nlohmann::json&)> callback);

private:
    ControlCenter& m_controller;
    Config& m_config;
    ScreenshotCallback m_screenshotCallback;
    ShellExecuteCallback m_shellExecuteCallback;
    std::function<void(const nlohmann::json&)>
        m_responseCallback;  // 异步执行管理
    std::atomic<int> m_activeExecutions{0};

    // 异步执行 shell 命令的内部方法
    void executeShellCommandAsync(const std::string& command,
                                  const std::string& session_id);
};

#endif  // COMMAND_DISPATCHER_H