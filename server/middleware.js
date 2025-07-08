// 中间件模块
// 集中管理Express中间件配置

const express = require('express');
const cors = require('cors');
const cookieParser = require('cookie-parser');
const config = require('./config');

/**
 * 配置基础中间件
 * @param {Express} app - Express应用实例
 */
function setupBasicMiddleware(app) {
  // JSON解析中间件
  app.use(express.json());

  // URL编码解析中间件
  app.use(express.urlencoded({ extended: true }));

  // Cookie解析中间件
  app.use(cookieParser());

  // CORS中间件
  app.use(cors({
    credentials: true, // 允许携带cookie
    origin: true
  }));

  // 静态文件服务
  if (config.BASE_PATH) {
    // 如果有子路径，需要在子路径下提供静态文件
    app.use(config.BASE_PATH, express.static(config.PUBLIC_DIR));
    app.use(config.BASE_PATH + '/uploads', express.static(config.UPLOADS_DIR));
  }
  // 同时保持根路径的静态文件服务（向后兼容）
  app.use(express.static(config.PUBLIC_DIR));
  app.use('/uploads', express.static(config.UPLOADS_DIR));
}

/**
 * 客户端ID解析中间件
 * 从URL参数中提取client_id并添加到req对象
 */
function clientIdMiddleware(req, res, next) {
  try {
    const url = new URL(req.url, `http://${req.headers.host}`);
    const clientId = url.searchParams.get('client_id');

    if (clientId) {
      req.clientId = clientId;
    } else {
      req.clientId = null;
    }

    next();
  } catch (e) {
    req.clientId = null;
    next(); // 保证不中断流程
  }
}

/**
 * 设置所有中间件
 * @param {Express} app - Express应用实例
 */
function setupAllMiddleware(app) {
  setupBasicMiddleware(app);
  app.use(clientIdMiddleware);
}

module.exports = {
  setupAllMiddleware,
  setupBasicMiddleware,
  clientIdMiddleware
};
