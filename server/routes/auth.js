// 认证路由模块
// 处理登录、登出和会话管理的路由

const express = require('express');
const { handleLogin, handleLogout, getSessionInfo } = require('../auth');

const router = express.Router();

/**
 * 登录接口
 */
router.post('/login', handleLogin);

/**
 * 登出接口
 */
router.post('/logout', handleLogout);

/**
 * 获取会话信息
 */
router.get('/session', getSessionInfo);

module.exports = router;
