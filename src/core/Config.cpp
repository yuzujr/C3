#include "core/Config.h"

#include <fstream>

#include "core/Logger.h"

using json = nlohmann::json;

bool Config::load(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            Logger::log2stderr("Error: Could not open config file: " + path);
            return false;
        }

        file >> json_data;

        if (!json_data.contains("api") || !json_data["api"].is_object()) {
            Logger::log2stderr(
                "Error: Missing or invalid 'api' section in config.");
            return false;
        }

        const auto& api = json_data["api"];

        upload_url = api.value("upload_url", upload_url);
        interval_seconds = api.value("interval_seconds", interval_seconds);
        max_retries = api.value("max_retries", max_retries);
        retry_delay_ms = api.value("retry_delay_ms", retry_delay_ms);
        if (api.contains("add_to_startup") &&
            api["add_to_startup"].is_boolean()) {
            add_to_startup = api["add_to_startup"];
        } else if (api.contains("add_to_startup") &&
                   !api["add_to_startup"].is_boolean()) {
            Logger::log2stderr(
                "Warning: 'add_to_startup' should be a boolean value. "
                "Defaulting to false.");
            add_to_startup = false;
        }

        if (upload_url.empty()) {
            Logger::log2stderr("Error: 'upload_url' is required.");
            return false;
        }
        if (interval_seconds <= 0) {
            Logger::log2stderr("Error: 'interval_seconds' must be > 0.");
            return false;
        }
        if (max_retries < 0 || retry_delay_ms < 0) {
            Logger::log2stderr("Error: retry parameters must be non-negative.");
            return false;
        }

        // 将last_config_time初始化为当前文件的最后修改时间
        // 确保在try_reload_config()中可以正确比较
        last_config_time = std::filesystem::last_write_time(path);
        return true;
    } catch (const std::exception& e) {
        Logger::log2stderr("Error loading config: " + std::string(e.what()));
        return false;
    }
}

bool Config::try_reload_config(const std::string& path) {
    auto current_time = std::filesystem::last_write_time(path);
    // 如果文件时间没有变化，则不重新加载
    if (current_time != last_config_time) {
        if (load(path)) {
            last_config_time = current_time;
            Logger::log2stdout("Config reloaded successfully");
            return true;
        } else {
            Logger::log2stderr("Failed to reload config");
        }
    }
    return false;
}

void Config::list() const {
    Logger::log2stdout("\tUpload URL: " + upload_url);
    Logger::log2stdout(
        "\tInterval_seconds: " + std::to_string(interval_seconds) + "s");
    Logger::log2stdout("\tMax retries: " + std::to_string(max_retries));
    Logger::log2stdout("\tRetry delay: " + std::to_string(retry_delay_ms) +
                       "ms");
    Logger::log2stdout("\tAdd to startup: " +
                       std::string(add_to_startup ? "true" : "false"));
}

void Config::list_default() {
    Logger::log2stdout("\tUpload URL: " + Config::default_upload_url);
    Logger::log2stdout("\tInterval_seconds: " +
                       std::to_string(Config::default_interval_seconds) + "s");
    Logger::log2stdout("\tMax retries: " +
                       std::to_string(Config::default_max_retries));
    Logger::log2stdout("\tRetry delay: " +
                       std::to_string(Config::default_retry_delay_ms) + "ms");
    Logger::log2stdout(
        "\tAdd to startup: " +
        std::string(Config::default_add_to_startup ? "true" : "false"));
}