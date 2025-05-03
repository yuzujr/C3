#ifndef CONFIG_H
#define CONFIG_H

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

#include "Logger.h"

class Config {
public:
    // 读取配置文件
    bool load(const std::string& path);

    // 尝试重新加载配置文件
    // 如果文件没有变化，则返回false
    // 如果文件发生变化，则更新配置，并返回true
    bool try_reload_config(const std::string& path);

    // 展示配置文件内容
    void list() const;

    // 上传地址
    std::string upload_url = "";
    // 上传间隔时间（秒）
    int interval_seconds = 60;
    // 最大重试次数
    int max_retries = 3;
    // 重试间隔时间（毫秒）
    int retry_delay_ms = 1000;

private:
    // 数据
    nlohmann::json json_data;
    // 上次读取配置文件的时间
    std::filesystem::file_time_type last_config_time;
};

#endif  // CONFIG_H