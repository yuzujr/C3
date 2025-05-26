#include "core/CommandDispatcher.h"

#include <format>
#include <string>

#include "core/Logger.h"

void CommandDispatcher::dispatchCommands(const nlohmann::json& commands) {
    for (const auto& cmd : commands["commands"]) {
        std::string command = cmd.value("type", "");
        if (command == "pause") {
            Logger::info("[command] pause");
            controller_.pause();
        } else if (command == "resume") {
            Logger::info("[command] resume");
            controller_.resume();
        } else if (command == "update_config") {
            Logger::info("[command] update config");
            if (!cmd.contains("data")) {
                Logger::warn(
                    "update_config command received without config data");
                continue;
            }
            if (config_.parseConfig(cmd["data"])) {
                config_.save("config.json");
                config_.updateLastWriteTime("config.json");
                config_.remote_changed = true;
                Logger::info("Config reloaded successfully");
                config_.list();
            } else {
                Logger::warn("Failed to parse config data, ignoring command");
            }
        } else if (command == "screenshot_now") {
            Logger::info("[command] screenshot now");
            controller_.requestScreenshot();
        } else if (command.empty()) {
            Logger::warn("[command] empty command");
        } else {
            Logger::warn(std::format("Unknown command: {}", command));
        }
    }
}