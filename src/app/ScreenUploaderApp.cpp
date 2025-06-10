#include "app/ScreenUploaderApp.h"

ScreenUploaderApp::ScreenUploaderApp()
    : m_running(true),
      m_config(),
      m_controller(),
      m_dispatcher(m_controller, m_config),
      m_wsClient() {
    init();
}

ScreenUploaderApp::~ScreenUploaderApp() {
    m_running = false;
    // 关闭 WebSocket 客户端
    m_wsClient.stop();
}

void ScreenUploaderApp::init() {
    // 初始化日志
    Logger::init(spdlog::level::info, spdlog::level::info);

    // 读取配置文件
    if (!m_config.load("config.json")) {
        Logger::error("Failed to load config.json");
        throw std::runtime_error("Failed to load config.json");
    }
    Logger::info("Config loaded successfully");
    m_config.list();

    // 应用配置设置
    applyConfigSettings(m_config);

    // 启用高 DPI 感知
    SystemUtils::enableHighDPI();
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

        // 检查配置文件是否有更新
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

        // 截取屏幕
        auto frame = ScreenCapturer::captureScreen();
        if (!frame.has_value()) {
            Logger::error("Failed to capture screen");
        } else {
            uploadImageWithRetry(ImageEncoder::encodeToJPEG(frame.value()),
                                 m_config);
        }

        if (m_controller.consumeScreenshotRequest()) {
            Logger::info("Screenshot triggered by remote command!");
        }

        // 等待下一次上传
        Logger::info("Waiting for next capture...\n");
        m_controller.interruptibleSleepUntil(
            start + std::chrono::seconds(m_config.interval_seconds));
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