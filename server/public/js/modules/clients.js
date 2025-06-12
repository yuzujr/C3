// 客户端管理模块
// 处理客户端列表的获取、选择和高亮显示
// 统一使用alias作为客户端标识符

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
        clientsList.innerHTML = ''; if (clients.length === 0) {
            clientsList.textContent = '无客户端在线';
            setSelectedClient(null);
            document.getElementById('commands').style.display = 'none';
            document.getElementById('screenshots').textContent = '请选择客户端';
            // 禁用终端功能 - 直接处理
            const buttons = ['cmdPwd', 'cmdLs', 'cmdDir', 'executeCustom'];
            const customInput = document.getElementById('customCommand');
            const terminalMessage = document.getElementById('terminalMessage');

            buttons.forEach(id => {
                const button = document.getElementById(id);
                if (button) {
                    button.disabled = true;
                }
            });

            if (customInput) {
                customInput.disabled = true;
            }

            if (terminalMessage) {
                terminalMessage.style.display = 'block';
            }
            return;
        }

        // 显示命令区域
        document.getElementById('commands').style.display = 'block';

        clients.forEach(client => {
            const div = document.createElement('div');
            div.textContent = `${client.alias} (在线)`;
            div.className = 'client-item';
            div.dataset.clientAlias = client.alias; // 存储客户端alias
            if (client.alias === selectedClient) {
                div.classList.add('selected');
            }
            div.onclick = () => selectClient(client.alias); // 使用alias
            clientsList.appendChild(div);
        });
    } catch (error) {
        console.error('获取客户端列表失败:', error);
        showToast('获取客户端列表失败');
    }
}

/**
 * 选择客户端
 * @param {string} clientAlias - 客户端别名
 */
export async function selectClient(clientAlias) {
    setSelectedClient(clientAlias);
    updateClientHighlight();

    document.getElementById('screenshots').textContent = '加载截图中...';

    // 显示命令按钮
    document.getElementById('cmdButtons').style.display = 'block';

    // 启用终端功能 - 直接处理而不是调用导入的函数
    const buttons = ['cmdPwd', 'cmdLs', 'cmdDir', 'executeCustom'];
    const customInput = document.getElementById('customCommand');
    const terminalMessage = document.getElementById('terminalMessage');

    buttons.forEach(id => {
        const button = document.getElementById(id);
        if (button) {
            button.disabled = false;
        }
    });

    if (customInput) {
        customInput.disabled = false;
    }

    if (terminalMessage) {
        terminalMessage.style.display = 'none';
    }

    // 显示配置区域
    const configSection = document.getElementById('configSection');
    const configMessage = document.getElementById('configMessage');
    if (configSection && configMessage) {
        configSection.style.display = 'block';
        configMessage.style.display = 'none';
    }

    // 并行加载截图和配置，避免阻塞
    try {
        const [,] = await Promise.allSettled([
            fetchScreenshots(clientAlias).catch(error => {
                console.error(`加载截图失败:`, error);
                const container = document.getElementById('screenshots');
                if (container) {
                    container.textContent = `加载截图失败: ${error.message}`;
                }
            }),
            loadClientConfig(clientAlias).catch(error => {
                console.error(`加载配置失败:`, error);
            })
        ]);
    } catch (error) {
        console.error('选择客户端时出错:', error);
    }
}

/**
 * 更新客户端高亮显示
 */
export function updateClientHighlight() {
    document.querySelectorAll('.client-item').forEach(div => {
        if (div.dataset.clientAlias === selectedClient) {
            div.classList.add('selected');
        } else {
            div.classList.remove('selected');
        }
    });
}
