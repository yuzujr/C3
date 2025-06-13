// 事件监听器模块
// 集中管理各种DOM事件监听器的设置

import { sendCommand, updateClientConfig } from './commands.js';

/**
 * 初始化命令按钮事件监听器
 */
export function initCommandListeners() {
    // 暂停命令按钮
    document.getElementById('pauseBtn')?.addEventListener('click', () => {
        sendCommand({ type: 'pause_screenshots' });
    });

    // 继续命令按钮
    document.getElementById('resumeBtn')?.addEventListener('click', () => {
        sendCommand({ type: 'resume_screenshots' });
    });

    // 立即截图按钮
    document.getElementById('screenshotBtn')?.addEventListener('click', () => {
        sendCommand({ type: 'take_screenshot' });
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
