#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include "core/Config.h"
#include "core/ControlCenter.h"
#include "nlohmann/json.hpp"

class CommandDispatcher {
public:
    CommandDispatcher(ControlCenter& controller, Config& config)
        : controller_(controller), config_(config) {}
    ~CommandDispatcher() = default;
    CommandDispatcher(const CommandDispatcher&) = delete;
    CommandDispatcher& operator=(const CommandDispatcher&) = delete;

    void dispatchCommands(const nlohmann::json& commands);

private:
    ControlCenter& controller_;
    Config& config_;
};

#endif  // COMMAND_DISPATCHER_H