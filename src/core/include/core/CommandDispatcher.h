#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include "core/Config.h"
#include "core/ControlCenter.h"
#include "nlohmann/json.hpp"

class CommandDispatcher {
public:
    CommandDispatcher(ControlCenter& controller, Config& config)
        : m_controller(controller), m_config(config) {}
    ~CommandDispatcher() = default;
    CommandDispatcher(const CommandDispatcher&) = delete;
    CommandDispatcher& operator=(const CommandDispatcher&) = delete;

    void dispatchCommands(const nlohmann::json& commands);

private:
    ControlCenter& m_controller;
    Config& m_config;
};

#endif  // COMMAND_DISPATCHER_H