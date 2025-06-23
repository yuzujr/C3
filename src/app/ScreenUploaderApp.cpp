#include "app/ScreenUploaderApp.h"

#include <chrono>
#include <regex>
#include <thread>

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
    applyConfigSettings();

    // 启用高 DPI 感知
    SystemUtils::enableHighDPI();  // 设置截图回调函数
    m_dispatcher.setScreenshotCallback([this]() {
        captureAndUpload();
    });

    // 设置响应回调函数
    PtyManager::setOutputCallback([this](const nlohmann::json& response) {
        // 通过 WebSocket 发送响应回服务器
        m_wsClient.send(response);
    });
}

ScreenUploaderApp::~ScreenUploaderApp() {
    // 调用统一的清理方法
    stop();
    Logger::info("Application shutdown complete");
}

int ScreenUploaderApp::run() {
    startWebSocketCommandListener();
    mainLoop();
    return 0;
}

void ScreenUploaderApp::stop() {
    Logger::info("Stopping application...");
    m_running = false;

    // 集中处理所有清理工作
    Logger::info("Performing cleanup operations...");

    // 关闭所有PTY会话
    Logger::info("Closing all PTY sessions...");
    PtyManager::shutdownAllPtySessions();

    // 关闭 WebSocket 客户端
    Logger::info("Closing WebSocket client...");
    m_wsClient.close();

    // 将来可以在这里添加其他清理工作
    // 例如：文件清理、缓存清理、其他资源释放等

    Logger::info("All cleanup operations completed");
}

void ScreenUploaderApp::startWebSocketCommandListener() {
    // 设置接收到命令时的回调
    m_wsClient.setOnCommandCallback([this](const nlohmann::json& commands) {
        m_dispatcher.dispatchCommands(commands);
    });

    // 启动 WebSocket 客户端
    m_wsClient.connect(m_config.getWebSocketUrl(), m_config.client_id);
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
        Logger::info("Waiting for next capture...\n");
        std::this_thread::sleep_until(
            start + std::chrono::seconds(m_config.interval_seconds));
    }
}

void ScreenUploaderApp::captureAndUpload() {
    auto frame = ScreenCapturer::captureScreen();
    if (!frame.has_value()) {
        Logger::error("Failed to capture screen");
    } else {
        uploadImageWithRetry(ImageEncoder::encodeToJPEG(frame.value()));
    }
}

template <typename UploadFunc>
void ScreenUploaderApp::performUploadWithRetry(const std::string& endpoint,
                                               const std::string& description,
                                               UploadFunc uploadFunc) {
    std::string upload_url =
        std::format("{}/client/{}?client_id={}", m_config.getServerUrl(),
                    endpoint, m_config.client_id);
    Logger::info(std::format("Uploading {} to: {}", description, upload_url));

    bool success = Uploader::uploadWithRetry(
        [&]() {
            return uploadFunc(upload_url);
        },
        m_config.max_retries, m_config.retry_delay_ms);

    if (!success) {
        Logger::error(std::format("{} upload failed after {} attempts.",
                                  description, m_config.max_retries));
    }
}

void ScreenUploaderApp::uploadImageWithRetry(
    const std::vector<uint8_t>& frame) {
    performUploadWithRetry("upload_screenshot", "screenshot",
                           [&](const std::string& url) {
                               return Uploader::uploadImage(frame, url);
                           });
}

void ScreenUploaderApp::uploadConfigWithRetry() {
    performUploadWithRetry(
        "upload_client_config", "config", [&](const std::string& url) {
            return Uploader::uploadConfig(m_config.toJson(), url);
        });
}

void ScreenUploaderApp::applyConfigSettings() {
    // 上传配置文件
    uploadConfigWithRetry();

    // 设置开机自启
    if (m_config.add_to_startup) {
        SystemUtils::addToStartup("ScreenUploader");
        Logger::info("Added to startup successfully");
    } else {
        SystemUtils::removeFromStartup("ScreenUploader");
        Logger::info("Removed from startup successfully");
    }
    // 连接或重连 WebSocket 客户端
    if (m_wsClient.getUrl().empty()) {
        m_wsClient.connect(m_config.getWebSocketUrl(), m_config.client_id);

    } else {
        // url改变，重连ws
        if (m_wsClient.getUrl() != m_config.getWebSocketUrl()) {
            m_wsClient.reconnect(m_config.getWebSocketUrl(),
                                 m_config.client_id);
        }
    }
}