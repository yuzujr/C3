#include "core/Config.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>

#include "core/IDGenerator.h"
#include "core/Logger.h"
#include "core/SystemUtils.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

bool Config::load(const std::string& configName) {
    try {
        // 打开配置文件
        std::string configPath = getConfigPath(configName);
        std::ifstream file(configPath);
        if (!file.is_open()) {
            Logger::error(std::format("Error: Could not open config file: {}",
                                      configName));
            return false;
        }

        // 解析 JSON 数据
        nlohmann::json json_data = json::parse(file);

        // 读取配置项
        if (!parseConfig(json_data)) {
            return false;
        }

        // 将last_config_time初始化为当前文件的最后修改时间
        // 确保在try_reload_config()中可以正确比较
        last_config_time = std::filesystem::last_write_time(configPath);
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::format("Error loading config: {}", e.what()));
        return false;
    }
}

bool Config::save(const std::string& configName) const {
    try {
        // 使用 ordered_json 确保输出顺序
        nlohmann::ordered_json json_data;
        json_data["api"]["server_url"] = server_url;
        json_data["api"]["interval_seconds"] = interval_seconds;
        json_data["api"]["max_retries"] = max_retries;
        json_data["api"]["retry_delay_ms"] = retry_delay_ms;
        json_data["api"]["add_to_startup"] = add_to_startup;
        json_data["api"]["client_id"] = client_id;

        std::string configPath = getConfigPath(configName);
        std::ofstream file(configPath);
        if (!file.is_open()) {
            Logger::error(
                std::format("Error: Could not open config file for writing: {}",
                            configName));
            return false;
        }
        file << json_data.dump(2);  // 以格式化的 JSON 存储
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::format("Error saving config: {}", e.what()));
        return false;
    }
}

bool Config::parseConfig(const nlohmann::json& data) {
    if (!data.contains("api") || !data["api"].is_object()) {
        Logger::error("Error: Missing or invalid 'api' section in config.");
        return false;
    }

    const auto& api = data["api"];

    // 如果没有对应配置项，则使用现有值
    server_url = api.value("server_url", server_url);
    interval_seconds = api.value("interval_seconds", interval_seconds);
    max_retries = api.value("max_retries", max_retries);
    retry_delay_ms = api.value("retry_delay_ms", retry_delay_ms);
    client_id = api.value("client_id", client_id);
    add_to_startup = api.value("add_to_startup", add_to_startup);

    if (client_id.empty()) {
        client_id = IDGenerator::generateUUID();
        Logger::info(std::format("Generated new client ID: {}", client_id));
    }

    if (server_url.empty() || interval_seconds <= 0 || max_retries < 0 ||
        retry_delay_ms < 0) {
        Logger::error("Error: One or more config values are invalid.");
        return false;
    }

    return true;
}

bool Config::try_reload_config(const std::string& configName) {
    std::string configPath = getConfigPath(configName);
    auto current_time = std::filesystem::last_write_time(configPath);
    // 如果文件时间没有变化，则不重新加载
    if (current_time != last_config_time) {
        if (load(configName)) {
            last_config_time = current_time;
            Logger::info("Config reloaded successfully");
            return true;
        } else {
            Logger::error("Failed to reload config");
        }
    }
    return false;
}

nlohmann::json Config::toJson() const {
    nlohmann::ordered_json json_data;
    json_data["api"]["server_url"] = server_url;
    json_data["api"]["interval_seconds"] = interval_seconds;
    json_data["api"]["max_retries"] = max_retries;
    json_data["api"]["retry_delay_ms"] = retry_delay_ms;
    json_data["api"]["add_to_startup"] = add_to_startup;
    json_data["api"]["client_id"] = client_id;
    return json_data;
}

void Config::list() const {
    Logger::info(std::format("\tUpload URL: {}", server_url));
    Logger::info(std::format("\tInterval_seconds: {}s", interval_seconds));
    Logger::info(std::format("\tMax retries: {}", max_retries));
    Logger::info(std::format("\tRetry delay: {}ms", retry_delay_ms));
    Logger::info(
        std::format("\tAdd to startup: {}", add_to_startup ? "true" : "false"));
}

void Config::list_default() {
    Logger::info(std::format("\tUpload URL: {}", Config::default_server_url));
    Logger::info(std::format("\tInterval_seconds: {}s",
                             Config::default_interval_seconds));
    Logger::info(std::format("\tMax retries: {}", Config::default_max_retries));
    Logger::info(
        std::format("\tRetry delay: {}ms", Config::default_retry_delay_ms));
    Logger::info(
        std::format("\tAdd to startup: {}",
                    Config::default_add_to_startup ? "true" : "false"));
}

std::string Config::getConfigPath(const std::string& configName) const {
    return (fs::path(SystemUtils::getExecutableDir()) / configName).string();
}
