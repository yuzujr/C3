// WebSocket连接管理模块
// 处理WebSocket服务器和连接管理

const WebSocket = require('ws');
const config = require('./config');
const { logWithTime, errorWithTime } = require('./logger');
const { getClients } = require('./clients');

// WebSocket服务器实例
let wsServer = null;

// 连接管理
const activeConnections = new Map(); // client_id -> WebSocket (客户端连接)
const webConnections = new Set(); // Web界面的WebSocket连接

/**
 * 初始化WebSocket服务器
 */
function initWebSocketServer() {
    wsServer = new WebSocket.Server({ port: config.WS_PORT });

    wsServer.on('connection', handleWebSocketConnection);
}

/**
 * 处理WebSocket连接
 * @param {WebSocket} ws - WebSocket连接
 * @param {IncomingMessage} req - HTTP请求对象
 */
function handleWebSocketConnection(ws, req) {
    try {
        const url = new URL(req.url, `http://${req.headers.host}`);
        const client_id = url.searchParams.get('client_id');
        const connection_type = url.searchParams.get('type'); // 'client' 或 'web'

        // Web 界面连接（用于接收实时通知）
        if (connection_type === 'web') {
            handleWebConnection(ws);
            return;
        }

        // 客户端连接（用于接收命令）
        if (!client_id) {
            ws.close(4000, '缺少 client_id');
            return;
        }

        handleClientConnection(ws, client_id);
    } catch (error) {
        errorWithTime('[WEBSOCKET] Error handling connection:', error);
        ws.close(4001, '连接处理错误');
    }
}

/**
 * 处理Web界面连接
 * @param {WebSocket} ws - WebSocket连接
 */
function handleWebConnection(ws) {
    webConnections.add(ws);
    logWithTime('[WEBSOCKET] Web client connected');

    ws.on('close', () => {
        webConnections.delete(ws);
        logWithTime('[WEBSOCKET] Web client disconnected');
    });

    ws.on('error', (error) => {
        errorWithTime('[WEBSOCKET] Web client error:', error);
        webConnections.delete(ws);
    });
}

/**
 * 处理客户端连接
 * @param {WebSocket} ws - WebSocket连接
 * @param {string} client_id - 客户端ID
 */
function handleClientConnection(ws, client_id) {
    const clients = getClients();
    const alias = clients[client_id] || client_id;

    activeConnections.set(alias, ws);
    logWithTime(`[WEBSOCKET] Client connected: ${alias} (${client_id})`);

    ws.on('close', () => {
        activeConnections.delete(alias);
        logWithTime(`[WEBSOCKET] Client disconnected: ${alias} (${client_id})`);
    });

    ws.on('error', (error) => {
        errorWithTime(`[WEBSOCKET] Client error for ${alias}:`, error);
        activeConnections.delete(alias);
    });
}

/**
 * 向Web客户端广播消息
 * @param {object} message - 要广播的消息对象
 */
function broadcastToWebClients(message) {
    const messageStr = JSON.stringify(message);

    webConnections.forEach(ws => {
        if (ws.readyState === WebSocket.OPEN) {
            try {
                ws.send(messageStr);
                logWithTime('[BROADCAST] Sent message to Web client:', message.type);
            } catch (error) {
                errorWithTime('[BROADCAST] Failed to send message to Web client:', error);
                webConnections.delete(ws);
            }
        }
    });
}

/**
 * 向指定客户端发送消息
 * @param {string} clientAlias - 客户端别名
 * @param {object} message - 要发送的消息
 * @returns {boolean} 是否发送成功
 */
function sendToClient(clientAlias, message) {
    const ws = activeConnections.get(clientAlias);

    if (ws && ws.readyState === WebSocket.OPEN) {
        try {
            ws.send(JSON.stringify(message));
            return true;
        } catch (error) {
            errorWithTime(`[WEBSOCKET] Failed to send message to ${clientAlias}:`, error);
            activeConnections.delete(clientAlias);
            return false;
        }
    }

    return false;
}

/**
 * 检查客户端是否在线
 * @param {string} clientAlias - 客户端别名
 * @returns {boolean} 是否在线
 */
function isClientOnline(clientAlias) {
    const ws = activeConnections.get(clientAlias);
    return ws && ws.readyState === WebSocket.OPEN;
}

/**
 * 获取在线客户端列表
 * @returns {Array<string>} 在线客户端别名列表
 */
function getOnlineClients() {
    return Array.from(activeConnections.keys()).filter(alias =>
        isClientOnline(alias)
    );
}

/**
 * 关闭WebSocket服务器
 */
function closeWebSocketServer() {
    if (wsServer) {
        wsServer.close();
        logWithTime('[WEBSOCKET] WebSocket server closed');
    }
}

module.exports = {
    initWebSocketServer,
    broadcastToWebClients,
    sendToClient,
    isClientOnline,
    getOnlineClients,
    closeWebSocketServer,
    activeConnections,
    webConnections
};
