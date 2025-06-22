// WebSocket 通信模块
// 处理实时通知和WebSocket连接管理

import { selectedClient, setWebSocket } from './state.js';
import { addNewScreenshot } from './screenshots.js';
import { handlePtyShellOutput } from './pty-terminal.js';  // 新的PTY处理函数
import { handleClientStatusChange } from './clients.js';

/**
 * 初始化 WebSocket 连接
 */
export function initWebSocket() {
    // 使用当前页面的端口，支持HTTP和WebSocket共享端口
    const wsUrl = `ws://${window.location.hostname}:${window.location.port}?type=web`;

    const webSocket = new WebSocket(wsUrl);
    setWebSocket(webSocket);

    webSocket.onopen = () => {
        console.log('WebSocket连接已建立');
    };

    webSocket.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            handleWebSocketMessage(data);
        } catch (error) {
            console.error('WebSocket消息解析失败:', error);
        }
    };

    webSocket.onclose = (event) => {
        console.log('WebSocket连接已关闭, 3秒后重连...');
        setTimeout(initWebSocket, 3000);
    };

    webSocket.onerror = (error) => {
        console.error('WebSocket连接错误:', error);
    };
}

/**
 * 处理 WebSocket 消息
 * @param {object} data - 接收到的消息数据
 */
function handleWebSocketMessage(data) {
    if (data.type === 'new_screenshot') {
        if (data.client_id === selectedClient) {
            addNewScreenshot(data.screenshot_url);
        }
    } else if (data.type === 'shell_output') {
        // 处理shell命令输出
        if (data.client === selectedClient) {
            // 直接使用PTY处理函数
            handlePtyShellOutput(data);
        }
    } else if (data.type === 'client_status_change') {
        // 处理客户端状态变化
        handleClientStatusChange(data.client, data.online);
    }
}
