// Net/WebSocketClient.cpp
#include "net/WebSocketClient.h"

#include <ixwebsocket/IXNetSystem.h>

#include <format>

#include "core/Logger.h"

WebSocketClient::WebSocketClient() {}

WebSocketClient::~WebSocketClient() {
    close();
    ix::uninitNetSystem();
}

std::string WebSocketClient::getUrl() const {
    std::string fullUrl = m_ws.getUrl();

    // 找到第三个斜杠的位置，截取基本URL
    size_t protocolEnd = fullUrl.find("://");
    if (protocolEnd != std::string::npos) {
        size_t pathStart = fullUrl.find("/", protocolEnd + 3);
        if (pathStart != std::string::npos) {
            return fullUrl.substr(0, pathStart);
        }
    }

    return fullUrl;  // 如果解析失败，返回原URL
}

void WebSocketClient::close() {
    m_ws.close();
}

void WebSocketClient::reconnect(const std::string& url,
                                const std::string& client_id,
                                bool skip_ssl_verification) {
    // 重新连接
    close();
    connect(url, client_id, skip_ssl_verification);
}

void WebSocketClient::send(const nlohmann::json& message) {
    if (m_ws.getReadyState() == ix::ReadyState::Open) {
        std::string message_str = message.dump();
        ix::WebSocketSendInfo result = m_ws.send(message_str);
        if (result.success) {
            Logger::debug(
                std::format("Sent message to server: {}", message_str));
        } else {
            Logger::error("Failed to send message to server");
        }
    } else {
        Logger::warn("WebSocket not connected, cannot send message");
    }
}

void WebSocketClient::connect(const std::string& url,
                              const std::string& client_id,
                              bool skip_ssl_verification) {
    std::string ws_url =
        std::format("{}/client/commands?client_id={}", url, client_id);
    m_ws.setUrl(ws_url);

    // 配置SSL/TLS设置
    if (ws_url.starts_with("wss://")) {
        ix::SocketTLSOptions tlsOptions;

        if (skip_ssl_verification) {
            Logger::warn(
                "SSL certificate verification disabled - unsafe for "
                "production");
            tlsOptions.caFile = "NONE";  // 跳过证书验证
        }

        m_ws.setTLSOptions(tlsOptions);
    } else {
    }

    ix::OnMessageCallback callback =
        [this](const ix::WebSocketMessagePtr& msg) {
            onMessage(*msg);
        };

    // 设置接收到消息时的回调
    m_ws.setOnMessageCallback(callback);

    // 连接 WebSocket
    m_ws.start();
}

void WebSocketClient::onMessage(const ix::WebSocketMessage& msg) const {
    if (msg.type == ix::WebSocketMessageType::Message) {
        try {
            // 解析服务端的 JSON 消息
            nlohmann::json commands = nlohmann::json::parse(msg.str);

            // 调用回调函数处理命令
            if (m_commandCallback) {
                m_commandCallback(commands);
            }
        } catch (const std::exception& e) {
            Logger::error(std::format("命令解析失败: {}", e.what()));
        }
    }
}

void WebSocketClient::setOnCommandCallback(
    std::function<void(const nlohmann::json&)> callback) {
    m_commandCallback = callback;
}
