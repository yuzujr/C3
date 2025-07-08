// 服务器配置模块
// 统一使用环境变量管理所有配置项

const path = require('path');
const crypto = require('crypto');

// 加载环境变量 (必须在最开始)
// 确保从正确的位置加载.env文件
require('dotenv').config({ path: path.join(__dirname, '.env') });

const { logWithTime } = require('./logger');

/**
 * 生成随机密钥
 */
function generateRandomSecret() {
  return crypto.randomBytes(32).toString('hex');
}

// 服务器配置
const config = {
  // 端口配置
  PORT: parseInt(process.env.PORT) || 3000,

  // 网络配置
  HOST: process.env.HOST || '0.0.0.0',

  // 反向代理配置
  BASE_PATH: process.env.BASE_PATH || '',

  // 目录配置
  PUBLIC_DIR: path.join(__dirname, 'public'),
  UPLOADS_DIR: process.env.UPLOAD_DIR ? path.join(__dirname, process.env.UPLOAD_DIR) : path.join(__dirname, 'uploads'),

  // 安全配置
  MAX_FILE_SIZE: parseInt(process.env.MAX_FILE_SIZE) || 10 * 1024 * 1024, // 10MB
  ALLOWED_FILE_TYPES: ['.jpg', '.jpeg', '.png', '.gif', '.bmp'],

  // 认证配置
  AUTH_ENABLED: process.env.AUTH_ENABLED !== 'false',
  AUTH_USERNAME: process.env.AUTH_USERNAME || 'admin',
  AUTH_PASSWORD: process.env.AUTH_PASSWORD || 'CHANGE_ME_PLEASE',
  SESSION_SECRET: process.env.SESSION_SECRET || generateRandomSecret(),
  SESSION_EXPIRE_HOURS: parseInt(process.env.SESSION_EXPIRE_HOURS) || 24,

  // 数据库配置
  DB_HOST: process.env.DB_HOST || 'localhost',
  DB_PORT: parseInt(process.env.DB_PORT) || 5432,
  DB_NAME: process.env.DB_NAME || 'c3_db',
  DB_USER: process.env.DB_USER || 'postgres',
  DB_PASSWORD: process.env.DB_PASSWORD || 'password',
  DB_LOGGING: process.env.DB_LOGGING === 'true',

  // 日志配置
  LOG_LEVEL: process.env.LOG_LEVEL || 'info'
};

// 配置验证和安全检查
function validateConfig() {
  const warnings = [];

  // 安全检查（只保留最重要的）
  if (config.AUTH_PASSWORD === 'your-secure-password-here' || config.AUTH_PASSWORD === 'admin123') {
    warnings.push('⚠️  Using default password! Please change AUTH_PASSWORD for security.');
  }

  if (config.AUTH_PASSWORD && config.AUTH_PASSWORD.length < 8) {
    warnings.push('⚠️  Password is too short! Recommend at least 8 characters.');
  }

  // 显示警告
  if (warnings.length > 0) {
    logWithTime('[CONFIG] Security Warnings:');
    warnings.forEach(warning => logWithTime(`[CONFIG] ${warning}`));
  }

  // 简化配置摘要
  logWithTime(`[CONFIG] Server: ${config.HOST}:${config.PORT} | Auth: ${config.AUTH_ENABLED ? 'ON' : 'OFF'} | DB: ${config.DB_HOST}:${config.DB_PORT}/${config.DB_NAME} | BASE_PATH: ${config.BASE_PATH || '(root)'}`);
}

// 执行配置验证
validateConfig();

module.exports = config;
