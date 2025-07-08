#ifndef URL_BUILDER_H
#define URL_BUILDER_H

#include <string>

// 前向声明
class Config;

/**
 * URL构建工具类
 * 负责根据配置信息构建各种类型的URL
 */
class URLBuilder {
public:
    /**
     * 构建HTTP URL
     * @param config 配置对象
     * @return HTTP URL字符串
     */
    static std::string buildHTTPUrl(const Config& config);

    /**
     * 构建WebSocket URL
     * @param config 配置对象
     * @return WebSocket URL字符串
     */
    static std::string buildWebSocketUrl(const Config& config);

    /**
     * 构建完整的API端点URL
     * @param config 配置对象
     * @param endpoint API端点路径（如 "/client/upload_screenshot"）
     * @return 完整的API URL
     */
    static std::string buildAPIUrl(const Config& config,
                                   const std::string& endpoint);

private:
    /**
     * 构建基础URL（协议+主机名+端口）
     * @param protocol 协议（http/https/ws/wss）
     * @param hostname 主机名
     * @param port 端口号
     * @param use_ssl 是否使用SSL
     * @return 基础URL字符串
     */
    static std::string buildBaseUrl(const std::string& protocol,
                                    const std::string& hostname, int port,
                                    bool use_ssl);
};

#endif  // URL_BUILDER_H
