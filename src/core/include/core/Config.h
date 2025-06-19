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

    // 配置文件模式
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

    // 转换为 JSON 格式
    nlohmann::ordered_json toJson() const;

    void updateLastWriteTime(const std::string& path);

    // 因为HardcodedConfig.h在配置文件模式中不存在，因此以下函数只在硬编码配置模式下声明
#ifdef USE_HARDCODED_CONFIG
    // 硬编码配置模式：初始化方法
    bool initHardcoded();

    // 硬编码配置模式：获取配置信息
    HardcodedConfig::ConfigInfo getHardcodedInfo() const;

    // 硬编码配置模式：显示硬编码配置
    void listHardcoded() const;

    // 硬编码配置模式：转换为 JSON 格式
    // 已经在配置文件模式中声明，因此不再重复声明
    // nlohmann::ordered_json toJson() const;
#endif

    // 通用方法：展示配置文件内容
    void list() const;

    // 通用方法：展示默认配置文件内容
    static void list_default();

    // 通用方法：判断是否使用硬编码配置
    static constexpr bool isHardcodedMode() {
#ifdef USE_HARDCODED_CONFIG
        return true;
#else
        return false;
#endif
    }

public:
    // 服务器主机名
    std::string hostname = std::string{default_hostname};
    // 服务器端口
    int port = default_port;
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

    // 辅助方法：构建服务器URL
    std::string getServerUrl() const {
        return std::format("http://{}:{}", hostname, port);
    }

    // 辅助方法：构建WebSocket URL
    std::string getWebSocketUrl() const {
        return std::format("ws://{}:{}", hostname, port);
    }

private:
    // 配置文件模式：配置文件路径
    std::string getConfigPath(const std::string& configName) const;

    // 配置文件模式：上次读取配置文件的时间
    std::filesystem::file_time_type last_write_time;

private:
    // 默认配置文件内容
    static constexpr std::string_view default_hostname = "127.0.0.1";
    static constexpr int default_port = 3000;
    static constexpr int default_interval_seconds = 60;
    static constexpr int default_max_retries = 3;
    static constexpr int default_retry_delay_ms = 1000;
    static constexpr bool default_add_to_startup = false;
    static constexpr std::string_view default_client_id = "";
};

#endif  // CONFIG_H