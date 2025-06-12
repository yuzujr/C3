#include "app/ScreenUploaderApp.h"

#include <regex>


ScreenUploaderApp::ScreenUploaderApp()
    : m_running(true),
      m_config(),
      m_controller(),
      m_dispatcher(m_controller, m_config),
      m_wsClient() {
    // 初始化日志
    Logger::init(spdlog::level::info, spdlog::level::info);

    // 根据编译配置选择配置模式
#ifdef USE_HARDCODED_CONFIG
    // 硬编码配置模式
    Logger::info("=== Initializing Hardcoded Configuration ===");
    if (!m_config.initHardcoded()) {
        Logger::error("Failed to initialize hardcoded config");
        throw std::runtime_error("Failed to initialize hardcoded config");
    }
    Logger::info("Hardcoded config initialized successfully");
    m_config.listHardcoded();
#else
    // 配置文件模式
    Logger::info("=== Starting with Config File Mode ===");
    if (!m_config.load("config.json")) {
        Logger::error("Failed to load config.json");
        throw std::runtime_error("Failed to load config.json");
    }
    Logger::info("Config loaded successfully");
    m_config.list();
#endif

    // 应用配置设置
    applyConfigSettings(m_config);

    // 启用高 DPI 感知
    SystemUtils::enableHighDPI();  // 设置截图回调函数
    m_dispatcher.setScreenshotCallback([this]() {
        takeScreenshotNow();
    });

    // 设置 Shell 命令执行回调函数
    m_dispatcher.setShellExecuteCallback(
        [this](const std::string& command, const std::string& session_id) {
            return executeShellCommand(command, session_id);
        });

    // 设置响应回调函数
    m_dispatcher.setResponseCallback([this](const nlohmann::json& response) {
        // 通过 WebSocket 发送响应回服务器
        m_wsClient.sendMessage(response);
    });
}

ScreenUploaderApp::~ScreenUploaderApp() {
    m_running = false;
    // 关闭 WebSocket 客户端
    m_wsClient.stop();
}

int ScreenUploaderApp::run() {
    startWebSocketCommandListener();
    mainLoop();
    return 0;
}

void ScreenUploaderApp::startWebSocketCommandListener() {
    // 设置接收到命令时的回调
    m_wsClient.setOnCommandCallback([this](const nlohmann::json& commands) {
        m_dispatcher.dispatchCommands(commands);
    });

    // 启动 WebSocket 客户端
    m_wsClient.start();
}

void ScreenUploaderApp::mainLoop() {
    // 进入主循环
    while (m_running) {
        // 如果接收到 pause 命令，暂停，等待服务器的 resume 命令
        m_controller.waitIfPaused();

        // 开始计时
        auto start = std::chrono::steady_clock::now();

        // 检查配置文件是否有更新（仅在配置文件模式下）
        if (!Config::isHardcodedMode()) {
            bool config_changed = m_config.try_reload_config("config.json");
            if (config_changed && m_config.remote_changed) {
                // 本地新修改覆盖远程修改，可能导致服务端没有达到预期效果
                Logger::warn("Local edits override remote changes");
            }
            if (config_changed || m_config.remote_changed) {
                // 本地或远程进行了修改
                m_config.remote_changed = false;
                applyConfigSettings(m_config);
            }
        } else {
            // 硬编码配置模式：处理远程配置更改
            if (m_config.remote_changed) {
                Logger::warn(
                    "Remote config change detected, but using hardcoded config "
                    "- ignoring");
                m_config.remote_changed = false;
            }
        }

        // 执行定时截图
        performScreenshotUpload();

        // 等待下一次上传
        Logger::info("Waiting for next capture...\n");
        std::this_thread::sleep_until(
            start + std::chrono::seconds(m_config.interval_seconds));
    }
}

void ScreenUploaderApp::takeScreenshotNow() {
    Logger::info("Taking immediate screenshot as requested");
    performScreenshotUpload();
}

void ScreenUploaderApp::performScreenshotUpload() {
    // 截取屏幕
    auto frame = ScreenCapturer::captureScreen();
    if (!frame.has_value()) {
        Logger::error("Failed to capture screen");
    } else {
        uploadImageWithRetry(ImageEncoder::encodeToJPEG(frame.value()),
                             m_config);
    }
}

void ScreenUploaderApp::uploadImageWithRetry(const std::vector<uint8_t>& frame,
                                             const Config& config) {
    std::string upload_url =
        std::format("{}/client/upload_screenshot?client_id={}",
                    config.server_url, config.client_id);
    Logger::info(std::format("Uploading screenshot to: {}", upload_url));
    bool success = Uploader::uploadWithRetry(
        [&]() {
            return Uploader::uploadImage(frame, upload_url);
        },
        config.max_retries, config.retry_delay_ms);
    if (!success) {
        Logger::error(std::format("Upload failed after {} attempts.\n",
                                  config.max_retries));
    }
}

void ScreenUploaderApp::uploadConfigWithRetry(const Config& config) {
    std::string upload_url =
        std::format("{}/client/upload_client_config?client_id={}",
                    config.server_url, config.client_id);
    Logger::info(std::format("Uploading config to: {}", upload_url));
    bool success = Uploader::uploadWithRetry(
        [&]() {
            return Uploader::uploadConfig(config.toJson(), upload_url);
        },
        config.max_retries, config.retry_delay_ms);
    if (!success) {
        Logger::error(std::format("Upload failed after {} attempts.\n",
                                  config.max_retries));
    }
}

void ScreenUploaderApp::applyConfigSettings(const Config& config) {
    // 上传配置文件
    uploadConfigWithRetry(config);

    // 设置开机自启
    if (config.add_to_startup) {
        SystemUtils::addToStartup("ScreenUploader");
        Logger::info("Added to startup successfully");
    } else {
        SystemUtils::removeFromStartup("ScreenUploader");
        Logger::info("Removed from startup successfully");
    }

    // 设置websocket地址
    if (!m_wsClient.getClientId().empty() && !m_wsClient.getWsUrl().empty()) {
        // 非首次调用
        if (m_wsClient.getClientId() != config.client_id ||
            m_wsClient.getWsUrl() != config.ws_url) {
            // ws_url或client_id改变，重连ws
            m_wsClient.reconnect(config.ws_url, config.client_id);
        }
    } else {
        // 首次调用
        // 设置ws url，client_id
        m_wsClient.setWsUrl(config.ws_url);
        m_wsClient.setClientId(config.client_id);
    }
}

nlohmann::json ScreenUploaderApp::executeShellCommand(
    const std::string& command, const std::string& session_id) {
    nlohmann::json result;

    try {
        Logger::info(std::format("Executing shell command: {} (session: {})",
                                 command, session_id));

        // 在 Windows 上使用 PowerShell 执行命令
        std::string full_command;

        if (command == "pwd" || command == "Get-Location") {
            // 显示当前工作目录
            full_command =
                "pwsh.exe -Command \"Get-Location | Select-Object "
                "-ExpandProperty Path\"";
        } else if (command.starts_with("ls") || command.starts_with("dir") ||
                   command.starts_with("Get-ChildItem")) {
            // 列出目录内容，使用更简洁的格式
            full_command =
                "pwsh.exe -Command \"Get-ChildItem | Format-Table Name, "
                "Length, LastWriteTime -AutoSize\"";
        } else {
            // 其他命令直接通过 PowerShell 执行
            full_command = std::format("pwsh.exe -Command \"{}\"", command);
        }

        // 执行命令并捕获输出
        FILE* pipe = _popen(full_command.c_str(), "r");
        if (!pipe) {
            result["success"] = false;
            result["error"] = "Failed to execute command";
            result["stderr"] = "Could not create process";
            result["exit_code"] = -1;
            return result;
        }

        // 读取输出
        std::string output;
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        int exit_code = _pclose(pipe);

        // 清理ANSI转义序列
        auto cleanAnsiEscapes = [](const std::string& input) -> std::string {
            std::string result = input;
            std::regex ansiRegex(R"(\x1b\[[0-9;]*[mGKH])");
            result = std::regex_replace(result, ansiRegex, "");
            return result;
        };

        std::string cleanOutput = cleanAnsiEscapes(output);

        // 构建结果
        result["success"] = (exit_code == 0);
        result["command"] = command;
        result["session_id"] = session_id;
        result["stdout"] = cleanOutput;
        result["stderr"] = "";  // PowerShell 的错误通常也会在 stdout 中
        result["exit_code"] = exit_code;
        result["finished"] = true;
        result["timestamp"] =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

        Logger::info(std::format("Command executed successfully. Exit code: {}",
                                 exit_code));

    } catch (const std::exception& e) {
        Logger::error(
            std::format("Exception while executing command: {}", e.what()));
        result["success"] = false;
        result["error"] = e.what();
        result["command"] = command;
        result["session_id"] = session_id;
        result["stdout"] = "";
        result["stderr"] = e.what();
        result["exit_code"] = -1;
        result["finished"] = true;
        result["timestamp"] =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
    }

    return result;
}