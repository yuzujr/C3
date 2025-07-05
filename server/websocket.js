// WebSocket连接管理模块
// 处理WebSocket服务器和连接管理

const WebSocket = require('ws');
const config = require('./config');
const { logWithTime, errorWithTime } = require('./logger');
const clientManager = require('./client-manager');

/**
 * 清理ANSI转义序列
 * @param {string} text - 包含ANSI转义序列的文本
 * @returns {string} 清理后的文本
 */
function cleanAnsiEscapes(text) {
    if (typeof text !== 'string') return text;
    // 移除ANSI颜色和格式代码
    return text.replace(/\x1b\[[0-9;]*[mGKH]/g, '');
}

// WebSocket服务器实例
let wsServer = null;

// 连接管理
const activeConnections = new Map(); // client_id -> WebSocket (客户端连接)
const webConnections = new Set(); // Web界面的WebSocket连接

/**
 * 初始化WebSocket服务器
 * @param {http.Server} server - HTTP服务器实例，WebSocket与HTTP共享端口
 */
function initWebSocketServer(server) {
    if (!server) {
        throw new Error('WebSocket server requires HTTP server instance to share port');
    }

    // 与HTTP服务器共享端口
    wsServer = new WebSocket.Server({ server });

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

    ws.on('close', () => {
        webConnections.delete(ws);
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
async function handleClientConnection(ws, client_id) {
    try {
        // 直接使用 client_id 作为连接标识符
        activeConnections.set(client_id, ws);
        logWithTime(`[CLIENT] Client connected: ${client_id}`);

        // 设置客户端在线状态
        try {
            const clientService = require('./services/client-service');
            await clientService.setClientOnline(client_id);
            clientService.clearClientCaches(client_id);
        } catch (error) {
            errorWithTime('[WEBSOCKET] Failed to set client online:', error);
        }

        // 广播客户端上线状态
        broadcastToWebClients({
            type: 'client_status_change',
            client_id: client_id,
            online: true
        });

        ws.on('close', () => {
            activeConnections.delete(client_id);
            logWithTime(`[CLIENT] Client disconnected: ${client_id}`);

            // 设置客户端离线状态并清除缓存
            try {
                const clientService = require('./services/client-service');
                clientService.setClientOffline(client_id);
                clientService.clearClientCaches(client_id);
            } catch (error) {
                errorWithTime('[WEBSOCKET] Failed to set client offline:', error);
            }

            // 广播客户端下线状态
            broadcastToWebClients({
                type: 'client_status_change',
                client_id: client_id,
                online: false
            });
        });

        ws.on('error', (error) => {
            errorWithTime(`[CLIENT] Client error for ${client_id}:`, error);
            activeConnections.delete(client_id);

            // 设置客户端离线状态并清除缓存
            try {
                const clientService = require('./services/client-service');
                clientService.setClientOffline(client_id);
                clientService.clearClientCaches(client_id);
            } catch (error) {
                errorWithTime('[WEBSOCKET] Failed to clear client cache on error:', error);
            }

            // 广播客户端下线状态
            broadcastToWebClients({
                type: 'client_status_change',
                client_id: client_id,
                online: false
            });
        });

        // 处理客户端发送的消息（如shell命令输出）
        ws.on('message', (data) => {
            try {
                const message = JSON.parse(data.toString());
                handleClientMessage(client_id, message);
            } catch (error) {
                errorWithTime(`[WEBSOCKET] Invalid message from ${client_id}:`, error);
            }
        });
    } catch (error) {
        errorWithTime('[WEBSOCKET] Error handling connection:', error);
        ws.close();
    }
}

/**
 * 处理客户端发送的消息
 * @param {string} client_id - 客户端ID
 * @param {object} message - 客户端消息
 */
function handleClientMessage(client_id, message) {
    switch (message.type) {
        case 'shell_output':
            handleShellOutput(client_id, message);
            break;
        default:
            // 尝试将未知消息作为shell输出处理
            if (message.command || message.output || message.stdout || message.result || message.data) {
                handleShellOutput(client_id, message);
            } else {
                logWithTime(`[WEBSOCKET] Unknown message type from ${client_id}:`, message.type);
            }
    }
}

/**
 * 处理shell命令输出
 * @param {string} client_id - 客户端ID
 * @param {object} message - shell输出消息
 */
function handleShellOutput(client_id, message) {
    // 从消息中提取输出数据，支持多种可能的数据结构
    const data = message.data || message || {};
    const rawOutput = data.stdout || data.output || data.result || message.stdout || message.output || message.result || '';
    const success = data.success || message.success || true;
    const exitCode = data.exit_code || data.exitCode || message.exit_code || message.exitCode || 0;

    // 转发shell输出到Web客户端 - 保留原始数据和清理后的数据
    const broadcastMessage = {
        type: 'shell_output',
        client_id: client_id,
        output: rawOutput,  // 保留原始输出（包含ANSI序列）
        clean_output: cleanAnsiEscapes(rawOutput), // 提供清理后的输出作为备选
        success: success,
        exit_code: exitCode
    };

    broadcastToWebClients(broadcastMessage);
}

/**
 * 向Web客户端广播消息
 * @param {object} message - 要广播的消息对象
 */
function broadcastToWebClients(message) {
    const messageStr = JSON.stringify(message); webConnections.forEach(ws => {
        if (ws.readyState === WebSocket.OPEN) {
            try {
                ws.send(messageStr);
            } catch (error) {
                errorWithTime('[BROADCAST] Failed to send message to Web client:', error);
                webConnections.delete(ws);
            }
        }
    });
}

/**
 * 向指定客户端发送消息
 * @param {string} client_id - 客户端ID
 * @param {object} message - 要发送的消息
 * @returns {boolean} 是否发送成功
 */
function sendToClient(client_id, message) {
    const ws = activeConnections.get(client_id);

    if (ws && ws.readyState === WebSocket.OPEN) {
        try {
            ws.send(JSON.stringify(message));
            return true;
        } catch (error) {
            errorWithTime(`[WEBSOCKET] Failed to send message to ${client_id}:`, error);
            activeConnections.delete(client_id);
            return false;
        }
    }

    return false;
}

/**
 * 检查客户端是否在线
 * @param {string} client_id - 客户端ID
 * @returns {boolean} 是否在线
 */
function isClientOnline(client_id) {
    const ws = activeConnections.get(client_id);
    return ws && ws.readyState === WebSocket.OPEN;
}

/**
 * 获取在线客户端列表
 * @returns {Array<string>} 在线客户端ID列表
 */
function getOnlineClients() {
    return Array.from(activeConnections.keys()).filter(client_id =>
        isClientOnline(client_id)
    );
}


/**
 * 关闭WebSocket服务器
 */
function closeWebSocketServer() {
    try {        // 关闭所有 Web 连接
        if (webConnections && webConnections.size > 0) {
            webConnections.forEach(ws => {
                try {
                    if (ws.readyState === WebSocket.OPEN) {
                        ws.close(1000, 'Server shutting down');
                    }
                } catch (error) {
                    // 静默处理连接关闭错误
                }
            });
            webConnections.clear();
        }

        // 关闭所有活跃的客户端连接
        if (activeConnections && activeConnections.size > 0) {
            activeConnections.forEach((ws, client_id) => {
                try {
                    if (ws.readyState === WebSocket.OPEN) {
                        ws.close(1000, 'Server shutting down');
                    }
                } catch (error) {
                    // 静默处理连接关闭错误
                }
            });
            activeConnections.clear();
        }
        // 关闭WebSocket服务器
        if (wsServer) {
            wsServer.close();
            wsServer = null;
        }
        // 清理客户端管理器
        try {
            if (clientManager && clientManager.cleanup) {
                clientManager.cleanup();
            }
        } catch (error) {
            // 静默处理清理错误
        }

    } catch (error) {
        logWithTime('[SHUTDOWN] Error during WebSocket server shutdown:', error);
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
