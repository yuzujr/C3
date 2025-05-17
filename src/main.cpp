#include <format>
#include <thread>

#include "core/core.h"
#include "net/net.h"

#ifdef _WIN32
#include <Windows.h>
#endif

void captureAndUpload(const Config& config) {
    Logger::info("Capturing screen...");
    cv::Mat frame = ScreenCapturer::captureScreen();

    if (frame.empty()) {
        Logger::error("Error: Failed to capture screen");
    } else {
        // 上传图像
        std::string upload_url = std::format(
            "{}/upload?client_id={}", config.server_url, config.client_id);
        Logger::info(std::format("Uploading to: {}", upload_url));
        bool success = Uploader::uploadWithRetry(
            frame, upload_url, config.max_retries, config.retry_delay_ms);
        if (!success) {
            Logger::error(std::format("Upload failed after {} attempts.\n",
                                      config.max_retries));
        }
    }
}

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

    // 设置开机自启
    if (config.add_to_startup) {
        SystemUtils::addToStartup("ScreenUploader");
        Logger::info("Added to startup successfully");
    } else {
        SystemUtils::removeFromStartup("ScreenUploader");
        Logger::info("Removed from startup successfully");
    }

    // 启用高 DPI 感知
    SystemUtils::enableHighDPI();

    // 命令控制器
    ControlCenter controlCenter;
    // 命令获取器
    CommandFetcher fetcher(config.server_url, config.client_id, controlCenter);
    // 启动命令轮询线程
    std::thread command_thread([&fetcher]() {
        while (true) {
            fetcher.fetchAndHandleCommands();
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    });

    // 进入主循环
    while (true) {
        // 如果接收到 pause 命令，暂停，等待服务器的 resume 命令
        controlCenter.waitIfPaused();

        // 开始计时
        auto start = std::chrono::steady_clock::now();

        // 检查配置文件是否有更新
        if (config.try_reload_config("config.json")) {
            config.list();
        }

        // 捕获屏幕图像并上传
        captureAndUpload(config);

        if (controlCenter.consumeScreenshotRequest()) {
            Logger::info("Screenshot triggered by remote command!");
        }

        // 等待下一次上传
        Logger::info("Waiting for next capture...\n");
        controlCenter.interruptibleSleepUntil(
            start + std::chrono::seconds(config.interval_seconds));
    }

    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main();
}
#endif
