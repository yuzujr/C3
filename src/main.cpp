#include <thread>

#include "Config.h"
#include "Logger.h"
#include "ScreenUploader.h"
#include "SystemUtils.h"

#ifdef _WIN32
#include <Windows.h>
#endif

int main() {
    // 读取配置文件
    Config config;
    if (!config.load("config.json")) {
        Logger::log2stderr("Failed to load config.json");
        return 1;
    }

    Logger::log2stdout("Config loaded successfully:");
    config.list();
    Logger::log2stdout("Default config:");
    Config::list_default();

#ifdef _WIN32
    if (config.add_to_startup) {
        SystemUtils::addToStartup("ScreenUploader");
        Logger::log2stdout("Added to startup successfully");
    } else {
        SystemUtils::removeFromStartup("ScreenUploader");
        Logger::log2stdout("Removed from startup successfully");
    }
#endif  // _WIN32

    // 启用高 DPI 感知
    SystemUtils::enableHighDPI();
    while (true) {
        // 检查配置文件是否有更新
        if (config.try_reload_config("config.json")) {
            config.list();
        }

        // 捕获屏幕图像
        Logger::log2stdout("Capturing screen...");
        cv::Mat frame = ScreenUploader::captureScreenMat();
        if (frame.empty()) {
            Logger::log2stderr("Error: Failed to capture screen");
            continue;
        }

        // 上传图像
        bool success = false;
        // 最多重试config.max_retries次
        for (int attempt = 1; attempt <= config.max_retries; ++attempt) {
            auto start = std::chrono::high_resolution_clock::now();
            success = ScreenUploader::uploadImage(frame, config.upload_url);
            if (success) break;

            Logger::log2stderr("Upload failed, retrying (" +
                               std::to_string(attempt) + "/" +
                               std::to_string(config.max_retries) + ")...");
            std::this_thread::sleep_until(
                start + std::chrono::milliseconds(config.retry_delay_ms));
        }

        if (!success) {
            Logger::log2stderr("Upload failed after " +
                               std::to_string(config.max_retries) +
                               " attempts.");
        }

        std::this_thread::sleep_for(
            std::chrono::seconds(config.interval_seconds));
    }

    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main();
}
#endif
