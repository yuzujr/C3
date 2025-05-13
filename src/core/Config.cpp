#include "core/Config.h"

#include <format>
#include <fstream>
#include <nlohmann/json.hpp>

#include "core/IDGenerator.h"
#include "core/Logger.h"

using json = nlohmann::json;

bool Config::load(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            Logger::error(
                std::format("Error: Could not open config file: {}", path));
            return false;
        }

        nlohmann::json json_data = json::parse(file);

        if (!json_data.contains("api") || !json_data["api"].is_object()) {
            Logger::error("Error: Missing or invalid 'api' section in config.");
            return false;
        }

        const auto& api = json_data["api"];

        upload_url = api.value("upload_url", default_upload_url);
        interval_seconds =
            api.value("interval_seconds", default_interval_seconds);
        max_retries = api.value("max_retries", default_max_retries);
        retry_delay_ms = api.value("retry_delay_ms", default_retry_delay_ms);
        client_id = api.value("client_id", default_client_id);
        if (client_id.empty()) {
            client_id = IDGenerator::generateUUID();
            Logger::info(std::format("Generated new client ID: {}", client_id));
            save(path);
        }
        if (api.contains("add_to_startup") &&
            api["add_to_startup"].is_boolean()) {
            add_to_startup = api["add_to_startup"];
        } else if (api.contains("add_to_startup") &&
                   !api["add_to_startup"].is_boolean()) {
            Logger::error(
                "Warning: 'add_to_startup' should be a boolean value. "
                "Defaulting to false.");
            add_to_startup = false;
        }

        if (upload_url.empty()) {
            Logger::error("Error: 'upload_url' is required.");
            return false;
        }
        if (interval_seconds <= 0) {
            Logger::error("Error: 'interval_seconds' must be > 0.");
            return false;
        }
        if (max_retries < 0 || retry_delay_ms < 0) {
            Logger::error("Error: retry parameters must be non-negative.");
            return false;
        }

        // 将last_config_time初始化为当前文件的最后修改时间
        // 确保在try_reload_config()中可以正确比较
        last_config_time = std::filesystem::last_write_time(path);
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::format("Error loading config: {}", e.what()));
        return false;
    }
}

bool Config::save(const std::string& path) const {
    try {
        // 使用 ordered_json 确保输出顺序
        nlohmann::ordered_json json_data;
        json_data["api"]["upload_url"] = upload_url;
        json_data["api"]["interval_seconds"] = interval_seconds;
        json_data["api"]["max_retries"] = max_retries;
        json_data["api"]["retry_delay_ms"] = retry_delay_ms;
        json_data["api"]["add_to_startup"] = add_to_startup;
        json_data["api"]["client_id"] = client_id;

        std::ofstream file(path);
        if (!file.is_open()) {
            Logger::error(std::format(
                "Error: Could not open config file for writing: {}", path));
            return false;
        }
        file << json_data.dump(2);  // 以格式化的 JSON 存储
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::format("Error saving config: {}", e.what()));
        return false;
    }
}

bool Config::try_reload_config(const std::string& path) {
    auto current_time = std::filesystem::last_write_time(path);
    // 如果文件时间没有变化，则不重新加载
    if (current_time != last_config_time) {
        if (load(path)) {
            last_config_time = current_time;
            Logger::info("Config reloaded successfully");
            return true;
        } else {
            Logger::error("Failed to reload config");
        }
    }
    return false;
}

void Config::list() const {
    Logger::debug(std::format("\tUpload URL: {}", upload_url));
    Logger::debug(std::format("\tInterval_seconds: {}s", interval_seconds));
    Logger::debug(std::format("\tMax retries: {}", max_retries));
    Logger::debug(std::format("\tRetry delay: {}ms", retry_delay_ms));
    Logger::debug(
        std::format("\tAdd to startup: {}", add_to_startup ? "true" : "false"));
}

void Config::list_default() {
    Logger::debug(std::format("\tUpload URL: {}", Config::default_upload_url));
    Logger::debug(std::format("\tInterval_seconds: {}s",
                              Config::default_interval_seconds));
    Logger::debug(
        std::format("\tMax retries: {}", Config::default_max_retries));
    Logger::debug(
        std::format("\tRetry delay: {}ms", Config::default_retry_delay_ms));
    Logger::debug(
        std::format("\tAdd to startup: {}",
                    Config::default_add_to_startup ? "true" : "false"));
}