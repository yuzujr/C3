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

    // 认证路由 (不需要认证)
    app.use('/auth', authRoutes);

    // 登录页面路由 (不需要认证)
    app.get('/login', (req, res) => {
        res.sendFile(path.join(config.PUBLIC_DIR, 'login.html'));
    });

    // 主页重定向到登录检查
    app.get('/', requireAuth, (req, res) => {
        res.sendFile(path.join(config.PUBLIC_DIR, 'index.html'));
    });

    // Web API路由 (需要认证)
    app.use('/web', requireAuth, webRoutes);

    // 客户端API路由 (不需要认证，客户端直接访问)
    app.use('/client', clientRoutes);

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
function startServer() {
    try {
        // 初始化上传模块
        initUploadModule();

        // 初始化WebSocket服务器
        initWebSocketServer();

        // 创建HTTP服务器
        const app = createApp();

        // 启动HTTP服务器
        const server = app.listen(config.HTTP_PORT, config.HOST, () => {
            logWithTime(`[INIT] HTTP Server running at http://127.0.0.1:${config.HTTP_PORT}`);
            logWithTime(`[INIT] WebSocket Server running at ws://127.0.0.1:${config.WS_PORT}`);
            logWithTime('[INIT] ScreenUploader Server started successfully');
        });

        // 关闭处理
        process.on('SIGINT', () => {
            logWithTime('[SHUTDOWN] Received SIGINT, shutting down...');

            server.close(() => {
                logWithTime('[SHUTDOWN] HTTP server closed');
                closeWebSocketServer();
                process.exit(0);
            });
        });

        process.on('SIGTERM', () => {
            logWithTime('[SHUTDOWN] Received SIGTERM, shutting down...');

            server.close(() => {
                logWithTime('[SHUTDOWN] HTTP server closed');
                closeWebSocketServer();
                process.exit(0);
            });
        });

        return server;
    } catch (error) {
        errorWithTime('[INIT] Failed to start server:', error);
        process.exit(1);
    }
}

/**
 * 应用入口点
 */
function main() {
    logWithTime('[INIT] Starting ScreenUploader Server...');
    logWithTime(`[INIT] Configuration: HTTP Port ${config.HTTP_PORT}, WS Port ${config.WS_PORT}`);

    startServer();
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
