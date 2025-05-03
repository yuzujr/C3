#include <thread>

#include "Config.h"
#include "Logger.h"
#include "ScreenUploader.h"

int main() {
    Config config;
    if (!config.load("config.json")) {
        Logger::log2stderr("Failed to load config.json");
        return 1;
    }

    Logger::log2stdout("Config loaded successfully");
    config.list();

    ScreenUploader screenUploader;
    while (true) {
        // 捕获屏幕图像
        Logger::log2stdout("Capturing screen...");
        cv::Mat frame = screenUploader.captureScreenMat();
        if (frame.empty()) {
            Logger::log2stderr("Error: Failed to capture screen");
            continue;
        }

        // 上传图像
        bool success = false;
        // 最多重试config.max_retries次
        for (int attempt = 1; attempt <= config.max_retries; ++attempt) {
            success = screenUploader.uploadImage(frame, config.upload_url);
            if (success) break;

            Logger::log2stderr("Upload failed, retrying (" +
                               std::to_string(attempt) + "/" +
                               std::to_string(config.max_retries) + ")...");
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config.retry_delay_ms));
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
