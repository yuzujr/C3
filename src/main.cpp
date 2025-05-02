#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>

#include "ScreenUploader.h"
#include "logger.h"

using json = nlohmann::json;

json config;

// 加载配置文件
bool load_config(const std::string& config_path) {
    try {
        std::ifstream config_file(config_path);
        if (!config_file.is_open()) {
            Logger::log2stderr("Error: Could not open config file: " +
                               config_path);
            return false;
        }

        config = json::parse(config_file);
        return true;
    } catch (const std::exception& e) {
        Logger::log2stderr("Error loading config: " + std::string(e.what()));
        return false;
    }
}

int main() {
    // 加载配置文件
    if (!load_config("config.json")) {
        Logger::log2stderr("Failed to load config.json");
        return 1;
    }
    try {
        Logger::log2stdout("Config loaded successfully");
        // 从配置读取URL
        std::string url = config["api"]["upload_url"].get<std::string>();
        Logger::log2stdout("\tUpload URL: " + url);

        // 从配置读取间隔时间,单位为秒
        int interval_seconds = config["api"]["interval_seconds"].get<int>();
        Logger::log2stdout(
            "\tInterval_seconds: " + std::to_string(interval_seconds) + "s");
        if (interval_seconds <= 0) {
            Logger::log2stderr("Interval_seconds must be greater than 0");
            return 1;
        }

        // 开始上传
        ScreenUploader screenUploader;
        while (true) {
            cv::Mat frame = screenUploader.captureScreenMat();  // 截取屏幕
            screenUploader.uploadImage(frame, url);             // 上传图像
            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        }
    } catch (const json::exception& e) {
        Logger::log2stderr("Error parsing config: " + std::string(e.what()));
        return 1;
    }

    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main();
}
#endif
