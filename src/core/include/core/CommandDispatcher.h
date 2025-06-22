#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>

#include "core/Config.h"
#include "core/ControlCenter.h"
#include "nlohmann/json.hpp"


// 前向声明
class ICommand;

class CommandDispatcher {
public:
    // 截图回调函数类型
    using ScreenshotCallback = std::function<void()>;

    // 命令执行结果类型
    struct CommandResult {
        bool success;
        std::string message;
        nlohmann::json data;

        CommandResult(bool s = true, const std::string& msg = "",
                      const nlohmann::json& d = {})
            : success(s), message(msg), data(d) {}
    };

    CommandDispatcher(ControlCenter& controller, Config& config);
    ~CommandDispatcher() = default;
    CommandDispatcher(const CommandDispatcher&) = delete;
    CommandDispatcher& operator=(const CommandDispatcher&) = delete;

    // 设置截图回调函数
    void setScreenshotCallback(ScreenshotCallback callback);

    // 分发接收到的命令
    void dispatchCommands(const nlohmann::json& message);

private:
    // 注册所有命令处理器
    void registerCommandHandlers();

    // 验证命令消息格式
    CommandResult validateCommandMessage(const nlohmann::json& message);

    // 执行命令
    CommandResult executeCommand(const std::string& commandType,
                                 const nlohmann::json& message);

    ControlCenter& m_controller;
    Config& m_config;
    ScreenshotCallback m_screenshotCallback;

    // 命令处理器映射表
    std::unordered_map<std::string, std::unique_ptr<ICommand>>
        m_commandHandlers;
};

// 命令接口
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual CommandDispatcher::CommandResult execute(
        const nlohmann::json& message) = 0;
};

#endif  // COMMAND_DISPATCHER_H