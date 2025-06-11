#include "core/CommandDispatcher.h"

#include <format>
#include <string>

#include "core/Logger.h"

void CommandDispatcher::setScreenshotCallback(ScreenshotCallback callback) {
    m_screenshotCallback = callback;
}

void CommandDispatcher::dispatchCommands(const nlohmann::json& commands) {
    for (const auto& cmd : commands["commands"]) {
        std::string command = cmd.value("type", "");
        if (command == "pause") {
            Logger::info("[command] pause");
            m_controller.pause();
        } else if (command == "resume") {
            Logger::info("[command] resume");
            m_controller.resume();
        } else if (command == "update_config") {
            Logger::info("[command] update config");
            if (!cmd.contains("data")) {
                Logger::warn(
                    "update_config command received without config data");
                continue;
            }
            if (m_config.parseConfig(cmd["data"])) {
                m_config.save("config.json");
                m_config.updateLastWriteTime("config.json");
                m_config.remote_changed = true;
                Logger::info("Config reloaded successfully");
                m_config.list();
            } else {
                Logger::warn("Failed to parse config data, ignoring command");
            }
        } else if (command == "screenshot_now") {
            Logger::info("[command] screenshot now");
            if (m_screenshotCallback) {
                m_screenshotCallback();
            } else {
                Logger::warn("Screenshot callback not set");
            }
        } else if (command.empty()) {
            Logger::warn("[command] empty command");
        } else {
            Logger::warn(std::format("Unknown command: {}", command));
        }
    }
}