// 客户端管理模块
// 处理客户端列表的获取、选择和高亮显示
// 统一使用alias作为客户端标识符

import { selectedClient, setSelectedClient, cachedClientList, setCachedClientList } from './state.js';
import { fetchScreenshots } from './screenshots.js';
import { loadClientConfig } from './commands.js';
import { showToast } from '../../toast/toast.js';

/**
 * 比较两个客户端列表是否相同
 * @param {Array} oldList - 旧的客户端列表
 * @param {Array} newList - 新的客户端列表
 * @returns {boolean} - 是否相同
 */
function isClientListSame(oldList, newList) {
    if (oldList.length !== newList.length) {
        return false;
    }

    for (let i = 0; i < oldList.length; i++) {
        const oldClient = oldList[i];
        const newClient = newList[i];

        if (oldClient.alias !== newClient.alias || oldClient.online !== newClient.online) {
            return false;
        }
    }

    return true;
}

/**
 * 根据客户端在线状态更新功能可用性
 * @param {boolean} isOnline - 客户端是否在线
 */
export function updateClientFeatures(isOnline) {
    // 导入标签页管理模块
    import('./tabs.js').then(({ setTabDisabled }) => {
        // 设置Shell和配置标签页的可用性
        setTabDisabled('shell', !isOnline);
        setTabDisabled('config', !isOnline);
    });

    if (isOnline) {
        // 在线客户端：启用所有功能

        // 显示命令按钮
        document.getElementById('cmdButtons').style.display = 'block';
        const commandMessage = document.getElementById('commandMessage');
        if (commandMessage) {
            commandMessage.style.display = 'none';
        }        // 启用终端功能 - 使用pty-terminal模块的统一方法
        import('./pty-terminal.js').then(({ updatePtyTerminalState }) => {
            updatePtyTerminalState(true);
        });

        // 显示配置区域
        const configSection = document.getElementById('configSection');
        const configMessage = document.getElementById('configMessage');
        if (configSection && configMessage) {
            configSection.style.display = 'block';
            configMessage.style.display = 'none';
        }

    } else {
        // 离线客户端：只允许查看截图

        // 隐藏命令按钮
        document.getElementById('cmdButtons').style.display = 'none';
        const commandMessage = document.getElementById('commandMessage');
        if (commandMessage) {
            commandMessage.textContent = '客户端离线，无法发送命令';
            commandMessage.style.display = 'block';
        }

        // 禁用终端功能 - 使用pty-terminal模块的统一方法
        import('./pty-terminal.js').then(({ updatePtyTerminalState }) => {
            updatePtyTerminalState(false);
        });

        // 隐藏配置区域
        const configSection = document.getElementById('configSection');
        const configMessage = document.getElementById('configMessage');
        if (configSection && configMessage) {
            configSection.style.display = 'none';
            configMessage.textContent = '客户端离线，无法修改配置';
            configMessage.style.display = 'block';
        }
    }
}

/**
 * 获取客户端列表
 */
export async function fetchClients() {
    try {
        const res = await fetch('/web/clients');

        // 检查HTTP响应状态
        if (!res.ok) {
            throw new Error(`HTTP ${res.status}: ${res.statusText}`);
        }

        const clients = await res.json();

        // 检查客户端列表是否有变化，但第一次加载时要强制更新DOM
        const isFirstLoad = cachedClientList.length === 0 && document.getElementById('clientsList').textContent === '加载中...';
        if (!isFirstLoad && isClientListSame(cachedClientList, clients)) {
            // 列表没有变化，直接返回，避免不必要的DOM更新
            return;
        }

        // 更新缓存
        setCachedClientList([...clients]);

        const clientsList = document.getElementById('clientsList');
        clientsList.innerHTML = ''; if (clients.length === 0) {
            clientsList.textContent = '无客户端';
            setSelectedClient(null);
            document.getElementById('commands').style.display = 'none';
            document.getElementById('screenshots').textContent = '请选择客户端';            // 禁用终端功能 - 使用pty-terminal模块的统一方法
            import('./pty-terminal.js').then(({ updatePtyTerminalState }) => {
                updatePtyTerminalState(false);
            });
            return;
        }// 检查是否有在线客户端
        const onlineClients = clients.filter(c => c.online);
        if (onlineClients.length === 0) {
            // 所有客户端都离线，但仍然显示客户端列表（可以查看截图）
            // 如果当前有选中的客户端，更新其功能状态
            if (selectedClient) {
                const selectedClientData = clients.find(c => c.alias === selectedClient);
                if (selectedClientData) {
                    updateClientFeatures(false); // 离线状态
                }
            }
        }// 显示命令区域
        document.getElementById('commands').style.display = 'block';        // 检查当前选中的客户端是否仍然在线
        if (selectedClient) {
            const selectedClientData = clients.find(c => c.alias === selectedClient);
            if (selectedClientData) {
                // 客户端存在，更新功能可用性
                updateClientFeatures(selectedClientData.online);
            } else {
                // 客户端不存在了，清除选择
                setSelectedClient(null);
                document.getElementById('cmdButtons').style.display = 'none';
                document.getElementById('screenshots').textContent = '请选择客户端';

                const commandMessage = document.getElementById('commandMessage');
                if (commandMessage) {
                    commandMessage.textContent = '请选择一个客户端';
                    commandMessage.style.display = 'block';
                }                // 禁用终端功能 - 使用pty-terminal模块的统一方法
                import('./pty-terminal.js').then(({ updatePtyTerminalState }) => {
                    updatePtyTerminalState(false);
                });

                // 隐藏配置区域
                const configSection = document.getElementById('configSection');
                const configMessage = document.getElementById('configMessage');
                if (configSection && configMessage) {
                    configSection.style.display = 'none';
                    configMessage.textContent = '请选择客户端';
                    configMessage.style.display = 'block';
                }
            }
        } clients.forEach(client => {
            const div = document.createElement('div');
            const onlineStatus = client.online ? '在线' : '离线';
            const statusClass = client.online ? 'online' : 'offline';            // 创建新的结构
            div.innerHTML = `
                <div class="client-name">
                    <span>${client.alias}</span>
                </div>
                <div class="client-status ${statusClass}">
                    ${onlineStatus}
                </div>
            `;

            div.className = `client-item ${statusClass}`;
            div.dataset.clientAlias = client.alias; // 存储客户端alias
            div.dataset.online = client.online; if (client.alias === selectedClient) {
                div.classList.add('selected');
            }

            // 所有客户端都可以被选择（包括离线的）
            div.onclick = () => selectClient(client.alias, client.online); clientsList.appendChild(div);
        });

        // 更新客户端管理列表
        try {
            const { updateClientManagementList } = await import('./client-management.js');
            updateClientManagementList(clients);
        } catch (error) {
            // 如果客户端管理模块未加载，忽略错误
            console.debug('Client management module not loaded');
        }
    } catch (error) {
        console.error('获取客户端列表失败:', error);
        showToast('获取客户端列表失败');

        // 更新DOM显示错误状态
        const clientsList = document.getElementById('clientsList');
        if (clientsList) {
            clientsList.innerHTML = '';
            clientsList.textContent = '获取客户端列表失败，请刷新页面重试';
        }

        // 清除选中状态和隐藏相关功能
        setSelectedClient(null);
        document.getElementById('commands').style.display = 'none';
        document.getElementById('screenshots').textContent = '请选择客户端';

        // 禁用终端功能
        import('./pty-terminal.js').then(({ updatePtyTerminalState }) => {
            updatePtyTerminalState(false);
        });
    }
}

/**
 * 选择客户端
 * @param {string} clientAlias - 客户端别名
 * @param {boolean} isOnline - 客户端是否在线
 */
export async function selectClient(clientAlias, isOnline = true) {
    // 如果选择的是同一个客户端，且在线状态没有变化，则不需要重新处理
    const currentClientData = cachedClientList.find(c => c.alias === selectedClient);
    if (selectedClient === clientAlias && currentClientData && currentClientData.online === isOnline) {
        // 只更新高亮显示即可
        updateClientHighlight();
        return;
    }

    // 记录是否是切换到不同的客户端
    const isDifferentClient = selectedClient !== clientAlias;

    setSelectedClient(clientAlias);
    updateClientHighlight();

    document.getElementById('screenshots').textContent = '加载截图中...';

    // 根据在线状态启用/禁用功能
    updateClientFeatures(isOnline);    // 重置终端工作目录（只在切换到不同客户端时）
    if (isDifferentClient) {
        import('./pty-terminal.js').then(({ resetPtyTerminal }) => {
            resetPtyTerminal();
        });
    }

    // 始终加载截图（在线和离线都可以查看）
    try {
        await fetchScreenshots(clientAlias);
    } catch (error) {
        console.error(`加载截图失败:`, error);
        const container = document.getElementById('screenshots');
        if (container) {
            container.textContent = `加载截图失败: ${error.message}`;
        }
    }

    // 只有在线客户端才加载配置
    if (isOnline) {
        try {
            await loadClientConfig(clientAlias);
        } catch (error) {
            console.error(`加载配置失败:`, error);
        }
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

/**
 * 处理WebSocket客户端状态变化通知
 * @param {string} clientAlias - 客户端别名
 * @param {boolean} isOnline - 是否在线
 */
export function handleClientStatusChange(clientAlias, isOnline) {
    // 更新缓存中客户端的状态
    const clientIndex = cachedClientList.findIndex(c => c.alias === clientAlias);
    if (clientIndex !== -1) {
        cachedClientList[clientIndex].online = isOnline;

        // 只有当状态变化的客户端是当前选中的客户端时，才更新功能可用性
        if (clientAlias === selectedClient) {
            updateClientFeatures(isOnline);

            // 更新标签页状态
            import('./tabs.js').then(({ setTabDisabled }) => {
                setTabDisabled('shell', !isOnline);
                setTabDisabled('config', !isOnline);
            });
        }

        // 更新DOM显示
        updateSingleClientDisplay(clientAlias, isOnline);
    }
}

/**
 * 更新单个客户端在DOM中的显示状态
 * @param {string} clientAlias - 客户端别名
 * @param {boolean} isOnline - 是否在线
 */
function updateSingleClientDisplay(clientAlias, isOnline) {
    const clientElement = document.querySelector(`[data-client-alias="${clientAlias}"]`);
    if (clientElement) {
        const statusText = isOnline ? '在线' : '离线';
        const statusClass = isOnline ? 'online' : 'offline';

        // 更新状态显示，保持原有的HTML结构
        const statusElement = clientElement.querySelector('.client-status');
        if (statusElement) {
            statusElement.textContent = statusText;
            statusElement.className = `client-status ${statusClass}`;
        }

        // 更新CSS类，保持选中状态
        clientElement.className = `client-item ${statusClass}`;
        if (clientAlias === selectedClient) {
            clientElement.classList.add('selected');
        }

        // 更新数据属性
        clientElement.dataset.online = isOnline;
    }
}
