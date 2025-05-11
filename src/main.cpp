#include <format>
#include <thread>

#include "core/core.h"
#include "net/net.h"

#ifdef _WIN32
#include <Windows.h>
#endif

int main() {
    // 初始化日志
    Logger::init();

    // 读取配置文件
    Config config;
    if (!config.load("config.json")) {
        Logger::error("Failed to load config.json");
        return 1;
    }

    Logger::info("Config loaded successfully:");
    config.list();
    Logger::debug("Default config:");
    Config::list_default();

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

    // 进入主循环
    while (true) {
        auto start = std::chrono::high_resolution_clock::now();

        // 检查配置文件是否有更新
        if (config.try_reload_config("config.json")) {
            config.list();
        }

        // 捕获屏幕图像
        Logger::info("Capturing screen...");
        cv::Mat frame = ScreenCapturer::captureScreen();
        if (frame.empty()) {
            Logger::error("Error: Failed to capture screen");
            continue;
        }

        // 上传图像
        bool success = Uploader::uploadWithRetry(frame, config.upload_url,
                                                 config.max_retries,
                                                 config.retry_delay_ms);
        if (!success) {
            Logger::error(std::format("Upload failed after {} attempts.\n",
                                      config.max_retries));
        }

        // 等待下一次上传
        Logger::info("Waiting for next capture...");
        std::this_thread::sleep_until(
            start + std::chrono::seconds(config.interval_seconds));
    }

    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main();
}
#endif
