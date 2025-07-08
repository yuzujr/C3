// 主应用入口文件
// 整合所有模块并启动服务器

const express = require('express');
const path = require('path');
const config = require('./config');
const { logWithTime, errorWithTime } = require('./logger');
const { setupAllMiddleware } = require('./middleware');
const { initWebSocketServer, closeWebSocketServer } = require('./websocket');
const { initUploadModule } = require('./upload');
const { requireAuth } = require('./auth');
const { initDatabase } = require('./database/init');

// 路由模块
const webRoutes = require('./routes/web');
const clientRoutes = require('./routes/client');
const authRoutes = require('./routes/auth');

/**
 * 创建Express应用
 */
function createApp() {
  const app = express();    // 设置中间件
  setupAllMiddleware(app);

  // 创建子路由器来处理基础路径
  const router = express.Router();

  // 认证路由 (不需要认证)
  router.use('/auth', authRoutes);

  // 页面渲染中间件 - 注入配置信息
  function injectConfig(req, res, next) {
    const originalSend = res.send;
    res.send = function (data) {
      if (typeof data === 'string' && data.includes('</head>')) {
        const configScript = `<script>window.APP_CONFIG = { BASE_PATH: '${config.BASE_PATH}' };</script>`;
        data = data.replace('</head>', configScript + '</head>');
      }
      originalSend.call(this, data);
    };
    next();
  }

  // 登录页面路由 (不需要认证)
  router.get('/login', injectConfig, (req, res) => {
    res.sendFile(path.join(config.PUBLIC_DIR, 'login.html'));
  });

  // 主页重定向到登录检查
  router.get('/', requireAuth, injectConfig, (req, res) => {
    res.sendFile(path.join(config.PUBLIC_DIR, 'index.html'));
  });

  // Web API路由 (需要认证)
  router.use('/web', requireAuth, webRoutes);

  // 客户端API路由 (不需要认证，客户端直接访问)
  router.use('/client', clientRoutes);

  // 如果设置了BASE_PATH，使用子路径
  if (config.BASE_PATH) {
    app.use(config.BASE_PATH, router);
  }

  // 同时保持根路径路由（向后兼容）
  app.use('/', router);

  // 错误处理中间件
  app.use((error, req, res, next) => {
    errorWithTime('[APP] Unhandled error:', error);
    res.status(500).send('Internal server error');
  });

  return app;
}

/**
 * 启动服务器
 */
async function startServer() {
  try {
    // 初始化数据库
    logWithTime('[INIT] Initializing database...');
    const dbInitialized = await initDatabase();
    if (!dbInitialized) {
      errorWithTime('[INIT] Database initialization failed');
      process.exit(1);
    }

    // 初始化上传模块
    initUploadModule();

    // 创建HTTP服务器
    const app = createApp();

    // 启动服务器并设置监听器
    const server = createServerWithFallback(app);

    // 设置优雅关闭
    setupGracefulShutdown(server);

    return server;
  } catch (error) {
    errorWithTime('[INIT] Failed to start server:', error);
    process.exit(1);
  }
}

/**
 * 创建服务器并处理端口冲突
 */
function createServerWithFallback(app) {
  const tryStartServer = (port, isRetry = false) => {
    const server = app.listen(port, config.HOST);

    server.on('listening', () => {
      const actualPort = server.address().port;
      initWebSocketServer(server);
      const portNote = isRetry ? ' (fallback port)' : '';
      logWithTime(`[INIT] Server running at http://127.0.0.1:${actualPort}${portNote}`);
    });

    server.on('error', (err) => {
      if (err.code === 'EADDRINUSE') {
        if (isRetry) {
          errorWithTime(`[INIT] Fallback port ${port} is also in use. Giving up.`);
          process.exit(1);
        } else {
          errorWithTime(`[INIT] Port ${port} is already in use`);
          const fallbackPort = port + 1;
          logWithTime(`[INIT] Trying fallback port ${fallbackPort}...`);
          tryStartServer(fallbackPort, true);
        }
      } else {
        errorWithTime(`[INIT] Server error:`, err);
        process.exit(1);
      }
    });

    return server;
  };

  return tryStartServer(config.PORT);
}

/**
 * 设置优雅关闭处理
 */
function setupGracefulShutdown(server) {
  let isShuttingDown = false;

  const gracefulShutdown = (signal) => {
    if (isShuttingDown) {
      logWithTime(`[SHUTDOWN] Already shutting down, ignoring ${signal}`);
      return;
    }

    isShuttingDown = true;
    logWithTime(`[SHUTDOWN] Received ${signal}, shutting down immediately...`);

    // 1. 首先关闭WebSocket服务器
    try {
      closeWebSocketServer();
    } catch (error) {
      logWithTime('[SHUTDOWN] Error closing WebSocket server:', error);
    }

    // 2. 快速关闭HTTP服务器，不等待连接关闭
    try {
      server.close(() => {
        logWithTime('[SHUTDOWN] HTTP server closed');
      });
    } catch (error) {
      logWithTime('[SHUTDOWN] Error closing HTTP server:', error);
    }

    // 3. 立即退出，不等待
    logWithTime('[SHUTDOWN] Shutdown completed');
    process.exit(0);
  };

  // 注册信号处理
  ['SIGINT', 'SIGTERM'].forEach(signal => {
    process.on(signal, () => gracefulShutdown(signal));
  });

  // 处理未捕获的异常
  process.on('uncaughtException', (error) => {
    logWithTime('[ERROR] Uncaught Exception:', error);
    gracefulShutdown('uncaughtException');
  });

  process.on('unhandledRejection', (reason, promise) => {
    logWithTime('[ERROR] Unhandled Rejection at:', promise, 'reason:', reason);
    gracefulShutdown('unhandledRejection');
  });
}

/**
 * 应用入口点
 */
async function main() {
  await startServer();
}

// 如果直接运行此文件，启动服务器
if (require.main === module) {
  main();
}

module.exports = {
  createApp,
  startServer,
  main
};
