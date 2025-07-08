#include "core/URLBuilder.h"

#include <format>

#include "core/Config.h"

std::string URLBuilder::buildHTTPUrl(const Config& config) {
    const std::string protocol = config.use_ssl ? "https" : "http";
    std::string url =
        buildBaseUrl(protocol, config.hostname, config.port, config.use_ssl);

    // 添加基础路径
    if (!config.base_path.empty()) {
        url += config.base_path;
    }

    return url;
}

std::string URLBuilder::buildWebSocketUrl(const Config& config) {
    const std::string protocol = config.use_ssl ? "wss" : "ws";
    std::string url =
        buildBaseUrl(protocol, config.hostname, config.port, config.use_ssl);

    // 添加基础路径
    if (!config.base_path.empty()) {
        url += config.base_path;
    }

    return url;
}

std::string URLBuilder::buildAPIUrl(const Config& config,
                                    const std::string& endpoint) {
    std::string base_url = buildHTTPUrl(config);

    // 如果endpoint不以'/'开头且base_url不以'/'结尾
    // 确保有斜杠分隔
    if (!endpoint.empty() && endpoint[0] != '/' && !base_url.empty() &&
        base_url.back() != '/') {
        base_url += '/';
    }

    base_url += endpoint;
    return base_url;
}

std::string URLBuilder::buildBaseUrl(const std::string& protocol,
                                     const std::string& hostname, int port,
                                     bool use_ssl) {
    const int default_port = use_ssl ? 443 : 80;

    std::string url = std::format("{}://{}", protocol, hostname);

    // 只有在非标准端口时才添加端口号
    if (port != default_port) {
        url += std::format(":{}", port);
    }

    return url;
}
