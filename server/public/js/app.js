// 主应用程序模块
// 应用程序的入口点和初始化逻辑

import { CLIENT_POLL_INTERVAL } from './modules/state.js';
import { fetchClients } from './modules/clients.js';
import { initImageModal } from './modules/modal.js';
import { initDeleteScreenshots } from './modules/delete.js';
import { initThemeToggle } from './modules/theme.js';
import { initWebSocket } from './modules/websocket.js';
import { initAllEventListeners } from './modules/events.js';
import { initAuth } from './modules/auth.js';

/**
 * 应用程序初始化函数
 */
function initApp() {
    // 首先初始化认证
    initAuth();

    // 初始化 WebSocket 连接
    initWebSocket();

    // 获取客户端列表
    fetchClients();

    // 定期更新客户端列表
    setInterval(fetchClients, CLIENT_POLL_INTERVAL);

    // 初始隐藏命令按钮
    document.getElementById('cmdButtons').style.display = 'none';

    // 初始化各种功能模块
    initImageModal();
    initDeleteScreenshots();
    initThemeToggle();
    initAllEventListeners();
}

// 当DOM加载完成时启动应用程序
window.addEventListener('load', initApp);
