#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <functional>

#include "core/Config.h"
#include "core/ControlCenter.h"
#include "nlohmann/json.hpp"


class CommandDispatcher {
public:
    // 截图回调函数类型
    using ScreenshotCallback = std::function<void()>;

    CommandDispatcher(ControlCenter& controller, Config& config)
        : m_controller(controller), m_config(config) {}
    ~CommandDispatcher() = default;
    CommandDispatcher(const CommandDispatcher&) = delete;
    CommandDispatcher& operator=(const CommandDispatcher&) = delete;

    // 设置截图回调函数
    void setScreenshotCallback(ScreenshotCallback callback);

    void dispatchCommands(const nlohmann::json& commands);

private:
    ControlCenter& m_controller;
    Config& m_config;
    ScreenshotCallback m_screenshotCallback;
};

#endif  // COMMAND_DISPATCHER_H