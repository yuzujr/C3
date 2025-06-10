// 事件监听器模块
// 集中管理各种DOM事件监听器的设置

import { sendCommand, updateClientConfig } from './commands.js';

/**
 * 初始化命令按钮事件监听器
 */
export function initCommandListeners() {
    // 命令按钮事件监听
    document.getElementById('commands').addEventListener('click', e => {
        if (e.target.tagName === 'BUTTON' && e.target.dataset.cmd) {
            const cmd = JSON.parse(e.target.dataset.cmd);
            sendCommand(cmd);
        }
    });

    // 更新配置按钮事件监听
    document.getElementById('updateConfigBtn').onclick = updateClientConfig;
}

/**
 * 初始化所有事件监听器
 */
export function initAllEventListeners() {
    initCommandListeners();
}
