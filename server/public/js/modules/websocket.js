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
        // 处理shell命令输出 - 现在使用 client_id
        if (data.client_id === selectedClient) {
            // 直接使用PTY处理函数
            handlePtyShellOutput(data);
        }
    } else if (data.type === 'client_status_change') {
        // 处理客户端状态变化 - 现在使用 client_id
        handleClientStatusChange(data.client_id, data.online);
    } else if (data.type === 'alias_updated') {
        // 处理别名更新
        handleAliasUpdated(data.client_id, data.oldAlias, data.newAlias);
    }
}

/**
 * 处理别名更新事件
 * @param {string} clientId - 客户端ID
 * @param {string} oldAlias - 旧别名
 * @param {string} newAlias - 新别名
 */
async function handleAliasUpdated(clientId, oldAlias, newAlias) {
    // 导入需要的模块
    const { selectedClient, cachedClientList, setCachedClientList, setSkipNextDOMRebuild } = await import('./state.js');
    const { fetchClients, updateClientHighlight } = await import('./clients.js');
    
    console.log(`别名更新: ${oldAlias} -> ${newAlias} (clientId: ${clientId})`);
    
    // 设置标志，让接下来的定时刷新跳过DOM重建
    setSkipNextDOMRebuild(true);
    
    // 立即更新缓存中的客户端别名
    const updatedClients = cachedClientList.map(client => {
        if (client.client_id === clientId) {
            return { ...client, alias: newAlias };
        }
        return client;
    });
    setCachedClientList(updatedClients);
    
    // 立即更新DOM中的别名显示
    const clientElement = document.querySelector(`[data-client-id="${clientId}"]`);
    if (clientElement) {
        const nameElement = clientElement.querySelector('.client-name');
        if (nameElement) {
            nameElement.textContent = newAlias;
        }
    }
    
    // 如果当前选中的客户端被重命名了，更新选中状态显示
    if (selectedClient === clientId) {
        // 更新客户端管理列表中的别名显示
        import('./client-management.js').then(({ updateClientManagementList }) => {
            updateClientManagementList(updatedClients);
        });
    }
}
