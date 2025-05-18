#include <atomic>
#include <thread>

#include "core/core.h"
#include "net/net.h"

// 程序运行状态
std::atomic<bool> running = true;

// 辅助函数
void uploadImageWithRetry(const cv::Mat& frame, const Config& config);
void uploadConfigWithRetry(const Config& config);
void applyConfigSettings(const Config& config);

int main() {
    // 初始化日志
    Logger::init(spdlog::level::info, spdlog::level::info);

    // 读取配置文件
    Config config;
    if (!config.load("config.json")) {
        Logger::error("Failed to load config.json");
        return 1;
    }
    Logger::info("Config loaded successfully");
    config.list();

    // 应用配置设置
    applyConfigSettings(config);

    // 启用高 DPI 感知
    SystemUtils::enableHighDPI();

    // 逻辑控制器
    ControlCenter controller;
    // 命令调度器
    CommandDispatcher dispatcher(controller, config);
    // 命令获取器
    CommandFetcher fetcher(config.server_url, config.client_id);
    // 启动命令轮询线程
    std::jthread command_thread([&fetcher, &dispatcher]() {
        while (running) {
            std::optional<nlohmann::json> commands = fetcher.fetchCommands();
            if (commands) {
                dispatcher.dispatchCommands(*commands);
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });

    // 进入主循环
    while (running) {
        // 如果接收到 pause 命令，暂停，等待服务器的 resume 命令
        controller.waitIfPaused();

        // 开始计时
        auto start = std::chrono::steady_clock::now();

        // 检查配置文件是否有更新
        if (config.try_reload_config("config.json")) {
            config.list();
            applyConfigSettings(config);
        }

        // 捕获屏幕图像并上传
        Logger::info("Capturing screen...");
        cv::Mat frame = ScreenCapturer::captureScreen();
        if (frame.empty()) {
            Logger::error("Error: Failed to capture screen");
        } else {
            uploadImageWithRetry(frame, config);
        }

        if (controller.consumeScreenshotRequest()) {
            Logger::info("Screenshot triggered by remote command!");
        }

        // 等待下一次上传
        Logger::info("Waiting for next capture...\n");
        controller.interruptibleSleepUntil(
            start + std::chrono::seconds(config.interval_seconds));
    }

    return 0;
}

void uploadImageWithRetry(const cv::Mat& frame, const Config& config) {
    std::string upload_url = std::format("{}/upload?client_id={}",
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

void uploadConfigWithRetry(const Config& config) {
    std::string upload_url = std::format("{}/client_config?client_id={}",
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

void applyConfigSettings(const Config& config) {
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
}