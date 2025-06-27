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
function handleClientConnection(ws, client_id) {
    const alias = clientManager.getAlias(client_id);

    // 如果客户端没有设置别名，将其添加到映射中（使用client_id作为别名）
    if (!clientManager.clientsMapping[client_id]) {
        clientManager.setClient(client_id, client_id);
    } activeConnections.set(alias, ws);
    logWithTime(`[CLIENT] Client connected: ${alias} (${client_id})`);


    // 广播客户端上线状态
    broadcastToWebClients({
        type: 'client_status_change',
        client: alias,
        online: true
    }); ws.on('close', () => {
        activeConnections.delete(alias);
        logWithTime(`[CLIENT] Client disconnected: ${alias} (${client_id})`);

        // 广播客户端下线状态
        broadcastToWebClients({
            type: 'client_status_change',
            client: alias,
            online: false
        });
    }); ws.on('error', (error) => {
        errorWithTime(`[CLIENT] Client error for ${alias}:`, error);
        activeConnections.delete(alias);


        // 广播客户端下线状态
        broadcastToWebClients({
            type: 'client_status_change',
            client: alias,
            online: false
        });
    });// 处理客户端发送的消息（如shell命令输出）
    ws.on('message', (data) => {
        try {
            const message = JSON.parse(data.toString());
            // 查找当前的alias（可能已经被更新）
            const currentAlias = findCurrentAlias(ws) || alias;
            handleClientMessage(currentAlias, message);
        } catch (error) {
            const currentAlias = findCurrentAlias(ws) || alias;
            errorWithTime(`[WEBSOCKET] Invalid message from ${currentAlias}:`, error);
        }
    });
}

/**
 * 查找WebSocket连接对应的当前别名
 * @param {WebSocket} ws - WebSocket连接
 * @returns {string|null} 当前别名，如果找不到返回null
 */
function findCurrentAlias(ws) {
    for (const [alias, connection] of activeConnections.entries()) {
        if (connection === ws) {
            return alias;
        }
    }
    return null;
}

/**
 * 处理客户端发送的消息
 * @param {string} alias - 客户端别名
 * @param {object} message - 客户端消息
 */
function handleClientMessage(alias, message) {
    switch (message.type) {
        case 'shell_output':
            handleShellOutput(alias, message);
            break;
        default:
            // 尝试将未知消息作为shell输出处理
            if (message.command || message.output || message.stdout || message.result || message.data) {
                handleShellOutput(alias, message);
            } else {
                logWithTime(`[WEBSOCKET] Unknown message type from ${alias}:`, message.type);
            }
    }
}

/**
 * 处理shell命令输出
 * @param {string} alias - 客户端别名
 * @param {object} message - shell输出消息
 */
function handleShellOutput(alias, message) {
    // 从消息中提取输出数据，支持多种可能的数据结构
    const data = message.data || message || {};
    const rawOutput = data.stdout || data.output || data.result || message.stdout || message.output || message.result || '';
    const success = data.success || message.success || true;
    const exitCode = data.exit_code || data.exitCode || message.exit_code || message.exitCode || 0;

    // 转发shell输出到Web客户端 - 保留原始数据和清理后的数据
    const broadcastMessage = {
        type: 'shell_output',
        client: alias,
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
            activeConnections.forEach((ws, clientId) => {
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

/**
 * 更新客户端别名映射
 * 当ClientManager中的别名发生变化时调用此函数
 * @param {string} oldAlias - 旧别名
 * @param {string} newAlias - 新别名
 */
function updateClientAlias(oldAlias, newAlias) {
    // 检查旧别名是否有活跃连接
    const ws = activeConnections.get(oldAlias);
    if (ws) {
        // 移除旧的映射
        activeConnections.delete(oldAlias);
        // 添加新的映射
        activeConnections.set(newAlias, ws);

        // 广播客户端状态变化（旧别名下线，新别名上线）
        broadcastToWebClients({
            type: 'client_status_change',
            client: oldAlias,
            online: false
        });

        broadcastToWebClients({
            type: 'client_status_change',
            client: newAlias,
            online: true
        });
    }
}

/**
 * 删除客户端连接
 * 当ClientManager中的客户端被删除时调用此函数
 * @param {string} alias - 客户端别名
 */
function removeClientConnection(alias) {
    const ws = activeConnections.get(alias);
    if (ws) {
        // 关闭WebSocket连接
        ws.close();
        // 移除映射
        activeConnections.delete(alias);

        // 广播客户端下线状态
        broadcastToWebClients({
            type: 'client_status_change',
            client: alias,
            online: false
        });
    }
}

module.exports = {
    initWebSocketServer,
    broadcastToWebClients,
    sendToClient,
    isClientOnline,
    getOnlineClients,
    closeWebSocketServer,
    updateClientAlias,
    removeClientConnection,
    activeConnections,
    webConnections
};
