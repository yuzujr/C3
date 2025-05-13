#ifndef CONFIG_H
#define CONFIG_H

#include <filesystem>
#include <string>

class Config {
public:
    // 读取配置文件
    bool load(const std::string& path);

    // 保存配置文件
    bool save(const std::string& path) const;

    // 尝试重新加载配置文件
    // 如果文件没有变化，则返回false
    // 如果文件发生变化，则更新配置，并返回true
    bool try_reload_config(const std::string& path);

    // 展示配置文件内容
    void list() const;

    // 展示默认配置文件内容
    static void list_default();

    // 上传地址
    std::string upload_url = default_upload_url;
    // 上传间隔时间（秒）
    int interval_seconds = default_interval_seconds;
    // 最大重试次数
    int max_retries = default_max_retries;
    // 重试间隔时间（毫秒）
    int retry_delay_ms = default_retry_delay_ms;
    // 是否加入开机启动项
    bool add_to_startup = default_add_to_startup;
    // 客户端ID
    std::string client_id = default_client_id;

private:
    // 默认配置文件内容
    static inline const std::string default_upload_url =
        "http://127.0.0.1:4000/upload";
    static constexpr int default_interval_seconds = 60;
    static constexpr int default_max_retries = 3;
    static constexpr int default_retry_delay_ms = 1000;
    static constexpr bool default_add_to_startup = false;
    static inline const std::string default_client_id = "";

    // 上次读取配置文件的时间
    std::filesystem::file_time_type last_config_time;
};

#endif  // CONFIG_H