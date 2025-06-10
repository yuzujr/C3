// 客户端管理模块
// 处理客户端列表的获取、选择和高亮显示

import { selectedClient, setSelectedClient } from './state.js';
import { fetchScreenshots } from './screenshots.js';
import { loadClientConfig } from './commands.js';
import { showToast } from '../../toast/toast.js';

/**
 * 获取客户端列表
 */
export async function fetchClients() {
    try {
        const res = await fetch('/web/clients');
        const clients = await res.json();

        const clientsList = document.getElementById('clientsList');
        clientsList.innerHTML = '';
        if (clients.length === 0) {
            clientsList.textContent = '无客户端在线';
            setSelectedClient(null);
            document.getElementById('commands').style.display = 'none';
            document.getElementById('screenshots').textContent = '请选择客户端';
            return;
        }

        clients.forEach(clientId => {
            const div = document.createElement('div');
            div.textContent = clientId;
            div.className = 'client-item';
            if (clientId === selectedClient) {
                div.classList.add('selected');
            }
            div.onclick = () => selectClient(clientId);
            clientsList.appendChild(div);
        });
    } catch (error) {
        console.error('获取客户端列表失败:', error);
    }
}

/**
 * 选择客户端
 * @param {string} clientId - 客户端ID
 */
export async function selectClient(clientId) {
    setSelectedClient(clientId);
    updateClientHighlight();

    document.getElementById('screenshots').textContent = '加载截图中...';
    document.getElementById('cmdButtons').style.display = 'block';

    await fetchScreenshots(clientId);
    await loadClientConfig(clientId);
}

/**
 * 更新客户端高亮显示
 */
export function updateClientHighlight() {
    document.querySelectorAll('.client-item').forEach(div => {
        if (div.textContent === selectedClient) {
            div.classList.add('selected');
        } else {
            div.classList.remove('selected');
        }
    });
}
