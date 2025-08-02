#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <functional>
#include <nlohmann/json.hpp>
#include <string>

#include "ixwebsocket/IXWebSocket.h"

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    // 关闭 WebSocket 连接
    void close();
    // 如果当前URL为空，直接连接
    // 如果当前连接的URL与新URL不同，则需要重新连接
    // 其余情况什么都不做
    void connectOrReconnect(const std::string& url,
                            bool skip_ssl_verification = false);

    // 发送消息到服务器
    void send(const nlohmann::json& message);

    // 设置命令接收后的回调
    void registerOnCommandCallback(
        std::function<void(const nlohmann::json&)> callback);

private:
    void onMessage(const ix::WebSocketMessage& msg) const;

    // 连接到 WebSocket 服务器
    void connect(const std::string& url, bool skip_ssl_verification = false);

    ix::WebSocket m_ws;
    std::function<void(const nlohmann::json&)> m_commandCallback;
};

#endif  // WEBSOCKET_CLIENT_H
