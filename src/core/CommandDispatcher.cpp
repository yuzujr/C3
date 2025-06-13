#include "core/CommandDispatcher.h"

#include <format>
#include <string>

#include "core/Logger.h"
#include "core/TerminalManager.h"

CommandDispatcher::~CommandDispatcher() {
    // 等待所有活跃的异步执行完成
    while (m_activeExecutions.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

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

void CommandDispatcher::dispatchCommands(const nlohmann::json& message) {
    // 验证消息格式
    if (!message.is_object()) {
        Logger::warn("Invalid command message: not a JSON object");
        return;
    }

    if (!message.contains("type") || !message["type"].is_string()) {
        Logger::warn(
            "Invalid command message: missing or invalid 'type' field");
        return;
    }

    std::string command = message["type"];

    if (command.empty()) {
        Logger::warn("[command] empty command");
        return;
    }

    Logger::info(std::format("[command] {}", command));
    if (command == "pause_screenshots") {
        m_controller.pause();
    } else if (command == "resume_screenshots") {
        m_controller.resume();
    } else if (command == "update_config") {
        if (!message.contains("data")) {
            Logger::warn("update_config command received without config data");
            return;
        }
        if (m_config.parseConfig(message["data"])) {
            m_config.save("config.json");
            m_config.updateLastWriteTime("config.json");
            m_config.remote_changed = true;
            Logger::info("Config reloaded successfully");
            m_config.list();
        } else {
            Logger::warn("Failed to parse config data, ignoring command");
        }
    } else if (command == "take_screenshot") {
        if (m_screenshotCallback) {
            m_screenshotCallback();
        } else {
            Logger::warn("Screenshot callback not set");
        }
    } else if (command == "shell_execute") {
        if (!message.contains("data")) {
            Logger::warn("shell_execute command received without data");
            return;
        }

        auto data = message["data"];
        std::string shell_command = data.value("command", "");
        std::string session_id = data.value("session_id", "default");
        if (shell_command.empty()) {
            Logger::warn("shell_execute command received with empty command");
            return;
        }
        if (m_shellExecuteCallback) {
            // 异步执行 shell 命令，避免阻塞 WebSocket 消息处理线程
            executeShellCommandAsync(shell_command, session_id);
        } else {
            Logger::warn("Shell execute callback not set");
        }
    } else if (command == "force_kill_session") {
        if (!message.contains("data")) {
            Logger::warn("force_kill_session command received without data");
            return;
        }

        auto data = message["data"];
        std::string session_id = data.value("session_id", "default");

        // 调用强制终止会话函数
        auto result = TerminalManager::forceKillSession(session_id);

        // 发送执行结果回服务器
        if (m_responseCallback) {
            nlohmann::json response = {{"type", "kill_session_result"},
                                       {"session_id", session_id},
                                       {"data", result}};
            m_responseCallback(response);
        }
    } else {
        Logger::warn(std::format("Unknown command: {}", command));
    }
}

void CommandDispatcher::executeShellCommandAsync(
    const std::string& command, const std::string& session_id) {
    // 增加活跃执行计数
    m_activeExecutions.fetch_add(1);

    // 创建分离的线程来执行命令
    std::thread([this, command, session_id]() {
        Logger::debug(std::format(
            "Starting async execution of shell command in session: {}",
            session_id));

        try {
            // 执行 shell 命令
            auto result = m_shellExecuteCallback(command, session_id);

            // 发送执行结果回服务器
            if (m_responseCallback) {
                nlohmann::json response = {{"type", "shell_output"},
                                           {"session_id", session_id},
                                           {"data", result}};
                m_responseCallback(response);
            }
        } catch (const std::exception& e) {
            Logger::error(
                std::format("Error executing shell command in session {}: {}",
                            session_id, e.what()));

            // 发送错误响应
            if (m_responseCallback) {
                nlohmann::json errorResponse = {
                    {"type", "shell_output"},
                    {"session_id", session_id},
                    {"data", nlohmann::json{{"success", false},
                                            {"error", e.what()},
                                            {"output", ""},
                                            {"exit_code", -1}}}};
                m_responseCallback(errorResponse);
            }
        }

        // 减少活跃执行计数
        m_activeExecutions.fetch_sub(1);

        Logger::debug(std::format(
            "Completed async execution of shell command in session: {}",
            session_id));
    }).detach();  // 分离线程，让其独立运行
    Logger::debug(
        std::format("Dispatched async shell command: {} (session: {})", command,
                    session_id));
}