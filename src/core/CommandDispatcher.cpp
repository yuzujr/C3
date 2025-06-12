#include "core/CommandDispatcher.h"

#include <format>
#include <string>

#include "core/Logger.h"

void CommandDispatcher::setScreenshotCallback(ScreenshotCallback callback) {
    m_screenshotCallback = callback;
}

void CommandDispatcher::setShellExecuteCallback(ShellExecuteCallback callback) {
    m_shellExecuteCallback = callback;
}

void CommandDispatcher::setResponseCallback(
    std::function<void(const nlohmann::json&)> callback) {
    m_responseCallback = callback;
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
        } else if (command == "shell_execute") {
            Logger::info("[command] shell execute");
            if (!cmd.contains("data")) {
                Logger::warn("shell_execute command received without data");
                continue;
            }

            auto data = cmd["data"];
            std::string shell_command = data.value("command", "");
            std::string session_id = data.value("session_id", "default");

            if (shell_command.empty()) {
                Logger::warn(
                    "shell_execute command received with empty command");
                continue;
            }

            if (m_shellExecuteCallback) {
                auto result = m_shellExecuteCallback(shell_command, session_id);

                // 发送执行结果回服务器
                if (m_responseCallback) {
                    nlohmann::json response = {{"type", "shell_output"},
                                               {"session_id", session_id},
                                               {"data", result}};
                    m_responseCallback(response);
                }
            } else {
                Logger::warn("Shell execute callback not set");
            }
        } else if (command.empty()) {
            Logger::warn("[command] empty command");
        } else {
            Logger::warn(std::format("Unknown command: {}", command));
        }
    }
}