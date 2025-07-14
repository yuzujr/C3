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

void WebSocketClient::close() {
    m_ws.close();
}

void WebSocketClient::connectOrReconnect(const std::string& url,

                                         bool skip_ssl_verification) {
    if (m_ws.getUrl().empty() || m_ws.getUrl() != url) {
        close();
        connect(url, skip_ssl_verification);
    }
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

                              bool skip_ssl_verification) {
    m_ws.setUrl(url);

    // 配置SSL/TLS设置
    if (url.starts_with("wss://")) {
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
