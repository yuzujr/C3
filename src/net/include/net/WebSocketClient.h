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

    // 获取当前 WebSocket 连接的 URL（不包含路径和查询参数）
    std::string getUrl() const;

    // 连接到 WebSocket 服务器
    void connect(const std::string& url, const std::string& client_id);

    // 关闭 WebSocket 连接
    void close();

    // 重连 WebSocket 连接
    void reconnect(const std::string& url, const std::string& client_id);

    // 发送消息到服务器
    void send(const nlohmann::json& message);

    // 设置命令接收后的回调
    void setOnCommandCallback(
        std::function<void(const nlohmann::json&)> callback);

private:
    void onMessage(const ix::WebSocketMessage& msg) const;

    ix::WebSocket m_ws;
    std::function<void(const nlohmann::json&)> m_commandCallback;
};

#endif  // WEBSOCKET_CLIENT_H
