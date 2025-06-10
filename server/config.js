// 服务器配置模块
// 集中管理所有配置项和常量

const path = require('path');

// 服务器配置
const config = {
    // 端口配置
    HTTP_PORT: 4000,
    WS_PORT: 8080,

    // 网络配置
    HOST: '0.0.0.0',

    // 目录配置
    PUBLIC_DIR: path.join(__dirname, 'public'),
    UPLOADS_DIR: path.join(__dirname, 'uploads'),

    // 安全配置
    MAX_FILE_SIZE: 10 * 1024 * 1024, // 10MB
    ALLOWED_FILE_TYPES: ['.jpg', '.jpeg', '.png', '.gif', '.bmp'],

    // 日志配置
    LOG_LEVEL: 'info'
};

module.exports = config;
