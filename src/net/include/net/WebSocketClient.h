#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <functional>
#include <nlohmann/json.hpp>
#include <string>

#include "ixwebsocket/IXWebSocket.h"


class WebSocketClient {
public:
    WebSocketClient(const std::string& ws_url,
                    const std::string& client_id);
    WebSocketClient();
    ~WebSocketClient();

    void setWsUrl(const std::string& ws_url);
    void setClientId(const std::string& client_id);
    const std::string& getWsUrl();
    const std::string& getClientId();

    // 启动 WebSocket 连接
    void start();

    // 停止 WebSocket 连接
    void stop();

    // 重连 WebSocket 连接
    void reconnect(const std::string& ws_url, const std::string& client_id);

    // 设置命令接收后的回调
    void setOnCommandCallback(
        std::function<void(const nlohmann::json&)> callback);

private:
    void onMessage(const ix::WebSocketMessage& msg);
    void connectToServer();

    ix::WebSocket m_ws;
    std::string m_ws_url;
    std::string m_client_id;
    std::function<void(const nlohmann::json&)> m_commandCallback;
};

#endif  // WEBSOCKET_CLIENT_H
