// WebSocket连接管理模块
// 简化的WebSocket服务器

const WebSocket = require('ws');
const { logWithTime, errorWithTime } = require('./logger');
const clientService = require('./services/client-service');

// WebSocket服务器实例
let wsServer = null;

// 连接管理
const activeConnections = new Map(); // client_id -> WebSocket
const webConnections = new Set(); // Web界面的WebSocket连接

/**
 * 清理ANSI转义序列
 */
function cleanAnsiEscapes(text) {
  if (typeof text !== 'string') return text;
  return text.replace(/\x1b\[[0-9;]*[mGKH]/g, '');
}

/**
 * 初始化WebSocket服务器
 */
function initWebSocketServer(server) {
  if (!server) {
    throw new Error('WebSocket server requires HTTP server instance');
  }

  wsServer = new WebSocket.Server({ server });
  wsServer.on('connection', handleWebSocketConnection);
  logWithTime('[WEBSOCKET] WebSocket server initialized');
}

/**
 * 处理WebSocket连接
 */
function handleWebSocketConnection(ws, req) {
  try {
    const url = new URL(req.url, `http://${req.headers.host}`);
    const client_id = url.searchParams.get('client_id');
    const connection_type = url.searchParams.get('type');

    if (connection_type === 'web') {
      handleWebConnection(ws);
    } else if (client_id) {
      handleClientConnection(ws, client_id);
    } else {
      ws.close(4000, '缺少 client_id');
    }
  } catch (error) {
    errorWithTime('[WEBSOCKET] Error handling connection:', error);
    ws.close(4001, '连接处理错误');
  }
}

/**
 * 处理Web界面连接
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
 */
async function handleClientConnection(ws, client_id) {
  try {
    activeConnections.set(client_id, ws);
    logWithTime(`[CLIENT] Connected: ${client_id}`);

    // 更新客户端状态 - 同时同步到client-service
    try {
      await clientService.setClientOnline(client_id);
      clientService.clearClientCaches(client_id);
      // 同步连接状态到client-service
      clientService.setActiveConnection(client_id, ws);
    } catch (error) {
      errorWithTime('[WEBSOCKET] Failed to update client status:', error);
    }

    // 广播上线状态
    broadcastToWebClients({
      type: 'client_status_change',
      client_id: client_id,
      online: true
    });

    ws.on('close', () => {
      activeConnections.delete(client_id);
      logWithTime(`[CLIENT] Disconnected: ${client_id}`);

      try {
        clientService.setClientOffline(client_id);
        clientService.clearClientCaches(client_id);
        // 同步移除连接状态
        clientService.removeActiveConnection(client_id);
      } catch (error) {
        errorWithTime('[WEBSOCKET] Failed to update client status:', error);
      }

      broadcastToWebClients({
        type: 'client_status_change',
        client_id: client_id,
        online: false
      });
    });

    ws.on('error', (error) => {
      errorWithTime(`[CLIENT] Error for ${client_id}:`, error);
      activeConnections.delete(client_id);

      try {
        clientService.setClientOffline(client_id);
        clientService.clearClientCaches(client_id);
        // 同步移除连接状态
        clientService.removeActiveConnection(client_id);
      } catch (error) {
        errorWithTime('[WEBSOCKET] Failed to update client status:', error);
      }

      broadcastToWebClients({
        type: 'client_status_change',
        client_id: client_id,
        online: false
      });
    });

    // 处理消息
    ws.on('message', (message) => {
      try {
        const data = JSON.parse(message);
        handleClientMessage(client_id, data);
      } catch (error) {
        errorWithTime(`[CLIENT] Invalid message from ${client_id}:`, error);
      }
    });

  } catch (error) {
    errorWithTime('[WEBSOCKET] Failed to handle client connection:', error);
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
    case 'kill_session_result':
      handleShellOutput(client_id, message);
      break;
    default:
      logWithTime(`[WEBSOCKET] Unknown message type from ${client_id}:`, message.type);
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

  // 转发shell输出到Web客户端 - 保留原始数据
  const broadcastMessage = {
    type: 'shell_output',
    client_id: client_id,
    output: rawOutput,  // 保留原始输出（包含ANSI序列）
    success: success,
    exit_code: exitCode
  };

  broadcastToWebClients(broadcastMessage);
}

/**
 * 发送命令到客户端
 */
function sendToClient(client_id, data) {
  const ws = activeConnections.get(client_id);
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(data));
    return true;
  }
  return false;
}

/**
 * 广播消息到所有Web客户端
 */
function broadcastToWebClients(data) {
  const message = JSON.stringify(data);
  webConnections.forEach(ws => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(message);
    }
  });
}

/**
 * 检查客户端是否在线
 */
function isClientOnline(client_id) {
  const ws = activeConnections.get(client_id);
  return ws && ws.readyState === WebSocket.OPEN;
}

/**
 * 获取在线客户端列表
 */
function getOnlineClients() {
  return Array.from(activeConnections.keys());
}

/**
 * 关闭WebSocket服务器
 */
function closeWebSocketServer() {
  if (wsServer) {
    wsServer.close();
    wsServer = null;
    logWithTime('[WEBSOCKET] WebSocket server closed');
  }

  activeConnections.clear();
  webConnections.clear();
}

module.exports = {
  initWebSocketServer,
  closeWebSocketServer,
  sendToClient,
  broadcastToWebClients,
  isClientOnline,
  getOnlineClients
};
