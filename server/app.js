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

        // 创建HTTP服务器
        const app = createApp();

        // 尝试启动服务器的函数
        const tryStartServer = (port, isRetry = false) => {
            const server = app.listen(port, config.HOST);

            server.on('listening', () => {
                const actualPort = server.address().port;
                // 初始化WebSocket服务器，与HTTP共享端口
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

        // 启动服务器
        const server = tryStartServer(config.PORT);        // 关闭处理
        let isShuttingDown = false; // 防止重复关闭

        const gracefulShutdown = (signal) => {
            if (isShuttingDown) {
                logWithTime(`[SHUTDOWN] Already shutting down, ignoring ${signal}`);
                return;
            }

            isShuttingDown = true;
            logWithTime(`[SHUTDOWN] Received ${signal}, shutting down gracefully...`);
            // 设置强制退出超时
            const forceExitTimer = setTimeout(() => {
                logWithTime('[SHUTDOWN] Force exit after timeout');
                process.exit(1);
            }, 3000); // 减少到3秒超时
            // 先关闭WebSocket服务器
            try {
                closeWebSocketServer();
            } catch (error) {
                logWithTime('[SHUTDOWN] Error closing WebSocket server:', error);
            }            // 关闭HTTP服务器
            const httpCloseTimer = setTimeout(() => {
                clearTimeout(forceExitTimer);
                process.exit(0);
            }, 1500); // 1.5秒后强制退出

            server.close((err) => {
                clearTimeout(httpCloseTimer);
                clearTimeout(forceExitTimer);

                if (err) {
                    logWithTime('[SHUTDOWN] Error closing HTTP server:', err);
                }

                process.exit(0);
            });            // 如果所有方法都失败，最后的保险机制
            setTimeout(() => {
                process.exit(1);
            }, 2500);
        }; process.on('SIGINT', () => gracefulShutdown('SIGINT'));
        process.on('SIGTERM', () => gracefulShutdown('SIGTERM'));

        // 处理未捕获的异常
        process.on('uncaughtException', (error) => {
            logWithTime('[ERROR] Uncaught Exception:', error);
            gracefulShutdown('uncaughtException');
        });

        process.on('unhandledRejection', (reason, promise) => {
            logWithTime('[ERROR] Unhandled Rejection at:', promise, 'reason:', reason);
            gracefulShutdown('unhandledRejection');
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
