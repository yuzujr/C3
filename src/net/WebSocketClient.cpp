// Net/WebSocketClient.cpp
#include "net/WebSocketClient.h"

#include <ixwebsocket/IXNetSystem.h>

#include <format>

#include "core/Logger.h"


WebSocketClient::WebSocketClient(const std::string& ws_url,
                                 const std::string& client_id)
    : m_ws_url(ws_url), m_client_id(client_id) {
    ix::initNetSystem();
}

WebSocketClient::WebSocketClient() {}

WebSocketClient::~WebSocketClient() {
    stop();
    ix::uninitNetSystem();
}

void WebSocketClient::setWsUrl(const std::string& ws_url) {
    m_ws_url = ws_url;
}
void WebSocketClient::setClientId(const std::string& client_id) {
    m_client_id = client_id;
}

const std::string& WebSocketClient::getWsUrl() {
    return m_ws_url;
}
const std::string& WebSocketClient::getClientId() {
    return m_client_id;
}

void WebSocketClient::start() {
    Logger::info(std::format("Connecting WebSocket on {}", m_ws_url));
    connectToServer();
}

void WebSocketClient::stop() {
    m_ws.close();
}

void WebSocketClient::reconnect(const std::string& server_url,
                                const std::string& client_id) {
    // 更新 URL 和 Client ID
    m_ws_url = server_url;
    m_client_id = client_id;

    // 重新连接
    stop();
    connectToServer();
}

void WebSocketClient::connectToServer() {
    std::string ws_url =
        std::format("{}/client/commands?client_id={}", m_ws_url, m_client_id);
    m_ws.setUrl(ws_url);

    ix::OnMessageCallback callback =
        [this](const ix::WebSocketMessagePtr& msg) {
            onMessage(*msg);
        };

    // 设置接收到消息时的回调
    m_ws.setOnMessageCallback(callback);

    // 连接 WebSocket
    m_ws.start();
}

void WebSocketClient::onMessage(const ix::WebSocketMessage& msg) {
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
