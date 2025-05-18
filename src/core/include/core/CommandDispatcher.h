#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include "core/Config.h"
#include "core/ControlCenter.h"
#include "nlohmann/json.hpp"

class CommandDispatcher {
public:
    CommandDispatcher(ControlCenter& center, Config& config)
        : controlCenter_(center), config_(config) {}

    void dispatchCommands(const nlohmann::json& commands);

private:
    ControlCenter& controlCenter_;
    Config& config_;
};

#endif  // COMMAND_DISPATCHER_H