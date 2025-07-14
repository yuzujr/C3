#include "net/URLBuilder.h"

#include <format>

#include "core/Config.h"

std::string URLBuilder::buildBaseUrl(const std::string& protocol,
                                     const Config& config) {
    const int defaultPort = config.use_ssl ? 443 : 80;

    std::string url = std::format("{}://{}", protocol, config.hostname);

    // 只有在非标准端口时才添加端口号
    if (config.port != defaultPort) {
        url += std::format(":{}", config.port);
    }

    // 添加基础路径
    url += config.base_path;

    // 确保以/结尾，避免非法的URL
    if (!url.ends_with("/")) {
        url += "/";
    }

    return url;
}

std::string URLBuilder::buildHTTPUrl(const Config& config,
                                     const std::string& endpoint) {
    const std::string protocol = config.use_ssl ? "https" : "http";

    return buildBaseUrl(protocol, config) + endpoint;
}

std::string URLBuilder::buildWebSocketUrl(const Config& config,
                                          const std::string& endpoint) {
    const std::string protocol = config.use_ssl ? "wss" : "ws";

    return buildBaseUrl(protocol, config) + endpoint;
}
