#include "core/CommandDispatcher.h"

#include <csignal>
#include <format>
#include <memory>
#include <string>

#include "core/Logger.h"
#include "core/PtyManager.h"

// ===========================================
// 具体命令实现类
// ===========================================

// 暂停截图命令
class PauseScreenshotsCommand : public ICommand {
private:
    UploadController& m_uploadController;

public:
    explicit PauseScreenshotsCommand(UploadController& controller)
        : m_uploadController(controller) {}
    CommandDispatcher::CommandResult execute(const nlohmann::json&) override {
        m_uploadController.pause();
        return {true, "Screenshots paused"};
    }
};

// 恢复截图命令
class ResumeScreenshotsCommand : public ICommand {
private:
    UploadController& m_uploadController;

public:
    explicit ResumeScreenshotsCommand(UploadController& controller)
        : m_uploadController(controller) {}
    CommandDispatcher::CommandResult execute(const nlohmann::json&) override {
        m_uploadController.resume();
        return {true, "Screenshots resumed"};
    }
};

// 更新配置命令
class UpdateConfigCommand : public ICommand {
private:
    Config& m_config;

public:
    explicit UpdateConfigCommand(Config& config) : m_config(config) {}

    CommandDispatcher::CommandResult execute(
        const nlohmann::json& message) override {
        if (!message.contains("data")) {
            return {false, "Missing config data"};
        }

        if (m_config.parseConfig(message["data"])) {
            m_config.save("config.json");
            m_config.updateLastWriteTime("config.json");
            m_config.remote_changed = true;
            Logger::info("Config reloaded successfully");
            m_config.list();
            return {true, "Config updated successfully"};
        } else {
            return {false, "Failed to parse config data"};
        }
    }
};

// 截图命令
class TakeScreenshotCommand : public ICommand {
private:
    CommandDispatcher::ScreenshotCallback m_callback;

public:
    explicit TakeScreenshotCommand(
        CommandDispatcher::ScreenshotCallback callback)
        : m_callback(callback) {}
    CommandDispatcher::CommandResult execute(const nlohmann::json&) override {
        if (m_callback) {
            m_callback();
            return {true, "Screenshot taken"};
        } else {
            return {false, "Screenshot callback not set"};
        }
    }
};

// PTY 调整大小命令
class PtyResizeCommand : public ICommand {
public:
    CommandDispatcher::CommandResult execute(
        const nlohmann::json& message) override {
        if (!message.contains("data")) {
            return {false, "Missing resize data"};
        }

        auto data = message["data"];
        std::string session_id = data.value("session_id", "default");
        int cols = data.value("cols", 80);
        int rows = data.value("rows", 24);

        // 调整 PTY 会话大小
        auto result =
            PtyManager::getInstance().resizePtySession(session_id, cols, rows);

        Logger::info(std::format("Resize PTY session {}: {}", session_id,
                                 result["message"].get<std::string>()));

        return {true, "PTY session resized", result};
    }
};

// 强制终止会话命令
class ForceKillSessionCommand : public ICommand {
public:
    CommandDispatcher::CommandResult execute(
        const nlohmann::json& message) override {
        if (!message.contains("data")) {
            return {false, "Missing session data"};
        }

        auto data = message["data"];
        std::string session_id = data.value("session_id", "default");

        auto result = PtyManager::getInstance().closePtySession(session_id);

        return {true, "Session killed", result};
    }
};

// PTY 输入命令（逐字符输入）
class PtyInputCommand : public ICommand {
public:
    CommandDispatcher::CommandResult execute(
        const nlohmann::json& message) override {
        if (!message.contains("data")) {
            return {false, "Missing PTY input data"};
        }

        auto data = message["data"];
        std::string input = data.value("input", "");
        std::string session_id = data.value("session_id", "default");

        if (input.empty()) {
            return {false, "Empty PTY input"};
        }

        // 直接将用户输入写入到 PTY 会话，不添加额外的换行符
        // 让 shell 自己处理输入的回显和处理
        PtyManager::getInstance().writeToPtySession(session_id, input);

        return {true,
                "PTY input sent",
                {{"session_id", session_id}, {"input_length", input.length()}}};
    }
};

// 创建PTY会话命令
class CreatePtySessionCommand : public ICommand {
public:
    CommandDispatcher::CommandResult execute(
        const nlohmann::json& message) override {
        if (!message.contains("data")) {
            return {false, "Missing PTY session data"};
        }

        auto data = message["data"];
        std::string session_id = data.value("session_id", "default");
        int cols = data.value("cols", 80);
        int rows = data.value("rows", 24);

        // 创建新的PTY会话
        auto result =
            PtyManager::getInstance().createPtySession(session_id, cols, rows);

        Logger::info(std::format("Create PTY session {}: {}", session_id,
                                 result["message"].get<std::string>()));

        return {true, "PTY session created", result};
    }
};

// 下线命令
class OfflineCommand : public ICommand {
public:
    CommandDispatcher::CommandResult execute(
        const nlohmann::json& message) override {
        // 检查是否有下线原因
        std::string reason = "Server requested offline";
        if (message.contains("data") && message["data"].contains("reason")) {
            reason = message["data"]["reason"];
        }

        Logger::info(std::format("Offline reason: {}", reason));

        // 发送SIGTERM信号给主线程
        raise(SIGTERM);

        return {true, "Offline command processed, shutting down"};
    }
};

// ===========================================
// CommandDispatcher 实现
// ===========================================

CommandDispatcher::CommandDispatcher(UploadController& controller,
                                     Config& config)
    : m_uploadController(controller), m_config(config) {
    registerCommandHandlers();
}

void CommandDispatcher::setScreenshotCallback(ScreenshotCallback callback) {
    m_screenshotCallback = callback;

    // 更新截图命令的回调
    m_commandHandlers["take_screenshot"] =
        std::make_unique<TakeScreenshotCommand>(callback);
}

void CommandDispatcher::dispatchCommands(const nlohmann::json& message) {
    // 验证消息格式
    auto validationResult = validateCommandMessage(message);
    if (!validationResult.success) {
        Logger::warn(std::format("Command validation failed: {}",
                                 validationResult.message));
        return;
    }

    std::string commandType = message["type"];

    // 频繁的 PTY 输入命令不需要记录日志
    if (commandType != "pty_input" && commandType != "pty_resize") {
        Logger::info(std::format("[command] {}", commandType));
    }

    // 执行命令
    auto result = executeCommand(commandType, message);

    if (!result.success) {
        Logger::warn(std::format("Command '{}' failed: {}", commandType,
                                 result.message));
    } else {
        // 频繁的 PTY 输入命令不需要记录日志
        if (commandType != "pty_input" && commandType != "pty_resize") {
            Logger::info(std::format("Command '{}' executed successfully: {}",
                                     commandType, result.message));
        }
    }
}

void CommandDispatcher::registerCommandHandlers() {
    // 注册所有命令处理器
    m_commandHandlers["pause_screenshots"] =
        std::make_unique<PauseScreenshotsCommand>(m_uploadController);
    m_commandHandlers["resume_screenshots"] =
        std::make_unique<ResumeScreenshotsCommand>(m_uploadController);
    m_commandHandlers["update_config"] =
        std::make_unique<UpdateConfigCommand>(m_config);
    m_commandHandlers["create_pty_session"] =
        std::make_unique<CreatePtySessionCommand>();
    m_commandHandlers["pty_input"] = std::make_unique<PtyInputCommand>();
    m_commandHandlers["pty_resize"] = std::make_unique<PtyResizeCommand>();
    m_commandHandlers["force_kill_session"] =
        std::make_unique<ForceKillSessionCommand>();
    m_commandHandlers["offline"] = std::make_unique<OfflineCommand>();

    // take_screenshot 命令需要在 setScreenshotCallback 中设置
}

CommandDispatcher::CommandResult CommandDispatcher::validateCommandMessage(
    const nlohmann::json& message) {
    if (!message.is_object()) {
        return {false, "Invalid command message: not a JSON object"};
    }

    if (!message.contains("type") || !message["type"].is_string()) {
        return {false,
                "Invalid command message: missing or invalid 'type' field"};
    }

    std::string commandType = message["type"];
    if (commandType.empty()) {
        return {false, "Empty command type"};
    }

    return {true, "Validation passed"};
}

CommandDispatcher::CommandResult CommandDispatcher::executeCommand(
    const std::string& commandType, const nlohmann::json& message) {
    auto it = m_commandHandlers.find(commandType);
    if (it == m_commandHandlers.end()) {
        return {false, std::format("Unknown command: {}", commandType)};
    }

    auto& [_, handler] = *it;
    try {
        return handler->execute(message);
    } catch (const std::exception& e) {
        return {false, std::format("Command execution failed: {}", e.what())};
    }
}