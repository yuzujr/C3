// 认证模块
// 处理前端登录状态检查和登出功能

import { buildUrl } from './path-utils.js';

/**
 * 检查登录状态
 * @returns {Promise<boolean>} 是否已登录
 */
export async function checkAuthStatus() {
  try {
    const response = await fetch(buildUrl('/auth/session'), {
      credentials: 'include'
    });
    const sessionInfo = await response.json();

    if (!sessionInfo.authEnabled) {
      // 认证被禁用
      return true;
    }

    if (!sessionInfo.authenticated) {
      // 未登录，重定向到登录页
      window.location.href = buildUrl('/login');
      return false;
    }

    return true;
  } catch (error) {
    console.error('检查登录状态失败:', error);
    // 发生错误时重定向到登录页
    window.location.href = buildUrl('/login');
    return false;
  }
}

/**
 * 登出
 */
export async function logout() {
  try {
    const response = await fetch(buildUrl('/auth/logout'), {
      method: 'POST',
      credentials: 'include'
    });

    if (response.ok) {
      // 登出成功，重定向到登录页
      window.location.href = buildUrl('/login');
    } else {
      console.error('登出失败');
    }
  } catch (error) {
    console.error('登出时出错:', error);
    // 即使出错也重定向到登录页
    window.location.href = buildUrl('/login');
  }
}

/**
 * 初始化认证相关功能
 */
export function initAuth() {
  // 检查登录状态
  checkAuthStatus();

  // 设置登出按钮事件
  const logoutBtn = document.getElementById('logoutBtn');
  if (logoutBtn) {
    logoutBtn.addEventListener('click', logout);
  }
}
