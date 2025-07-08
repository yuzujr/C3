#ifndef CONFIG_H
#define CONFIG_H

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

// 配置模式编译选项
// 定义 USE_HARDCODED_CONFIG 宏来启用硬编码配置模式
#ifdef USE_HARDCODED_CONFIG
#include "core/HardcodedConfig.h"
#endif

class Config {
public:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // ========================================
    // 配置文件模式声明
    // ========================================
    bool load(const std::string& path);

    // 保存配置文件
    bool save(const std::string& path) const;

    // 读取配置文件
    // 如果没有对应配置项，则使用现有值
    bool parseConfig(const nlohmann::json& data);

    // 尝试重新加载配置文件
    // 如果文件没有变化，则返回false
    // 如果文件发生变化，则更新配置，并返回true
    bool try_reload_config(const std::string& path);

    void updateLastWriteTime(const std::string& path);

    // ========================================
    // 硬编码配置模式声明
    // ========================================
#ifdef USE_HARDCODED_CONFIG

    // 初始化方法
    bool initHardcoded();

    // 获取配置信息
    HardcodedConfig::ConfigInfo getHardcodedInfo() const;

    // 显示硬编码配置
    void listHardcoded() const;
#endif

    // ========================================
    // 通用方法声明（两种模式都使用）
    // ========================================

    // 当前配置内容转换为 JSON 格式
    nlohmann::ordered_json toJson() const;

    // 展示配置文件内容
    void list() const;

    // 展示默认配置文件内容
    static void list_default();

    // 判断是否使用硬编码配置
    static constexpr bool isHardcodedMode() {
#ifdef USE_HARDCODED_CONFIG
        return true;
#else
        return false;
#endif
    }

public:
    // ========================================
    // 配置内容
    // ========================================
    // 服务器主机名
    std::string hostname = std::string{default_hostname};
    // 服务器端口
    int port = default_port;
    // 基础路径，用于反向代理部署（如"/c3"）
    std::string base_path = std::string{default_base_path};
    // 是否使用SSL/TLS加密 (HTTPS/WSS)
    bool use_ssl = default_use_ssl;
    // 是否跳过SSL证书验证 (仅用于测试)
    bool skip_ssl_verification = default_skip_ssl_verification;
    // 上传间隔时间（秒）
    int interval_seconds = default_interval_seconds;
    // 最大重试次数
    int max_retries = default_max_retries;
    // 重试间隔时间（毫秒）
    int retry_delay_ms = default_retry_delay_ms;
    // 是否加入开机启动项
    bool add_to_startup = default_add_to_startup;
    // 客户端ID
    std::string client_id = std::string{default_client_id};

    // 是否远程修改了配置文件
    bool remote_changed = false;

private:
    // 配置文件模式：配置文件路径
    std::string getConfigPath(const std::string& configName) const;

private:
    // 默认配置文件内容
    static constexpr std::string_view default_hostname = "127.0.0.1";
    static constexpr int default_port = 3000;
    static constexpr std::string_view default_base_path = "";
    static constexpr bool default_use_ssl = false;
    static constexpr bool default_skip_ssl_verification = false;
    static constexpr int default_interval_seconds = 60;
    static constexpr int default_max_retries = 3;
    static constexpr int default_retry_delay_ms = 1000;
    static constexpr bool default_add_to_startup = false;
    static constexpr std::string_view default_client_id = "";

    // 配置文件模式：上次写入配置文件的时间
    std::filesystem::file_time_type last_write_time;
};

#endif  // CONFIG_H