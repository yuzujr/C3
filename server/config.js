// 服务器配置模块
// 集中管理所有配置项和常量

// 加载环境变量 (必须在最开始)
require('dotenv').config();

const path = require('path');
const fs = require('fs');
const crypto = require('crypto');
const { logWithTime, errorWithTime } = require('./logger');

/**
 * 生成随机密钥
 */
function generateRandomSecret() {
    return crypto.randomBytes(32).toString('hex');
}

/**
 * 加载外部配置文件
 */
function loadExternalConfig() {
    const configPath = path.join(__dirname, 'server.config.json'); if (fs.existsSync(configPath)) {
        try {
            const externalConfig = JSON.parse(fs.readFileSync(configPath, 'utf8'));
            logWithTime('[INIT] Loaded external configuration from server.config.json');
            return externalConfig;
        } catch (error) {
            errorWithTime('[INIT] Warning: Failed to parse server.config.json:', error.message);
            return {};
        }
    }

    return {};
}

/**
 * 创建默认的外部配置文件模板
 */
function createDefaultConfigTemplate() {
    const configPath = path.join(__dirname, 'server.config.json');

    if (!fs.existsSync(configPath)) {
        const defaultConfig = {
            "auth": {
                "enabled": true,
                "username": "admin",
                "password": "admin123",
                "sessionSecret": generateRandomSecret(),
                "sessionExpireHours": 24
            }, "server": {
                "port": 3000,
                "host": "0.0.0.0"
            }
        }; try {
            fs.writeFileSync(configPath, JSON.stringify(defaultConfig, null, 4));
            logWithTime('[CONFIG] Created default configuration template: server.config.json');
            logWithTime('[CONFIG] ⚠️  IMPORTANT: Please edit server.config.json to set your password!');
        } catch (error) {
            errorWithTime('[CONFIG] Failed to create default config file:', error.message);
        }
    }
}

// 加载外部配置
const externalConfig = loadExternalConfig();

// 如果没有外部配置文件，创建模板
if (Object.keys(externalConfig).length === 0) {
    createDefaultConfigTemplate();
}

// 服务器配置
const config = {    // 端口配置 (优先使用环境变量，然后是外部配置，最后是默认值)
    PORT: parseInt(process.env.PORT) || externalConfig.server?.port || 3000,

    // 网络配置
    HOST: process.env.HOST || externalConfig.server?.host || '0.0.0.0',

    // 目录配置
    PUBLIC_DIR: path.join(__dirname, 'public'),
    UPLOADS_DIR: path.join(__dirname, 'uploads'),

    // 安全配置
    MAX_FILE_SIZE: 10 * 1024 * 1024, // 10MB
    ALLOWED_FILE_TYPES: ['.jpg', '.jpeg', '.png', '.gif', '.bmp'],

    // 认证配置 (优先使用环境变量，然后是外部配置，最后是默认值)
    AUTH_ENABLED: process.env.AUTH_ENABLED === 'false' ? false : (externalConfig.auth?.enabled !== false),
    AUTH_USERNAME: process.env.AUTH_USERNAME || externalConfig.auth?.username || 'admin',
    AUTH_PASSWORD: process.env.AUTH_PASSWORD || externalConfig.auth?.password || 'CHANGE_ME_PLEASE',
    SESSION_SECRET: process.env.SESSION_SECRET || externalConfig.auth?.sessionSecret || generateRandomSecret(),
    SESSION_EXPIRE_HOURS: parseInt(process.env.SESSION_EXPIRE_HOURS) || externalConfig.auth?.sessionExpireHours || 24,

    // 日志配置
    LOG_LEVEL: process.env.LOG_LEVEL || 'info'
};

// 配置验证和安全检查
function validateConfig() {
    const warnings = [];
    const errors = [];

    // 检查密码安全性
    if (config.AUTH_PASSWORD === 'CHANGE_ME_PLEASE' || config.AUTH_PASSWORD === 'admin123') {
        warnings.push('⚠️  Using default password! Please change AUTH_PASSWORD for security.');
    }

    if (config.AUTH_PASSWORD && config.AUTH_PASSWORD.length < 8) {
        warnings.push('⚠️  Password is too short! Recommend at least 8 characters.');
    }

    // 检查会话密钥
    if (config.SESSION_SECRET.length < 32) {
        warnings.push('⚠️  SESSION_SECRET is too short! Should be at least 32 characters.');
    }

    // 检查生产环境配置
    // 在Docker环境中，绑定0.0.0.0是正常的，因为有容器网络隔离
    const isDockerEnv = process.env.DOCKER_ENV === 'true' || fs.existsSync('/.dockerenv');
    if (config.HOST === '0.0.0.0' && process.env.NODE_ENV === 'production' && !isDockerEnv) {
        warnings.push('⚠️  Binding to 0.0.0.0 in production. Consider restricting to specific IP.');
    }    // 显示警告
    if (warnings.length > 0) {
        logWithTime('[CONFIG] Security Warnings:');
        warnings.forEach(warning => logWithTime(`  ${warning}`));
    }

    // 显示错误
    if (errors.length > 0) {
        errorWithTime('\n[CONFIG] Configuration Errors:');
        errors.forEach(error => errorWithTime(`  ❌ ${error}`));
        errorWithTime('');
        process.exit(1);
    }
}

// 执行配置验证
validateConfig();

module.exports = config;
