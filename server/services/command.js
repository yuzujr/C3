// 命令服务模块
// 处理客户端命令相关的业务逻辑

const fs = require('fs');
const path = require('path');
const config = require('../config');
const { logWithTime, errorWithTime, getBeijingTime } = require('../logger');
const { getClients } = require('../clients');

/**
 * 获取客户端配置
 * @param {string} clientId - 客户端ID
 * @returns {object|null} 配置对象或null
 */
function getClientConfig(clientId) {
    const configPath = path.join(config.UPLOADS_DIR, clientId, 'config.json');

    if (!fs.existsSync(configPath)) {
        return null;
    }

    try {
        const raw = fs.readFileSync(configPath, 'utf-8').trim();
        const configData = raw ? JSON.parse(raw) : {};
        return configData.api || null;
    } catch (error) {
        errorWithTime(`[CONFIG] Failed to read config for ${clientId}:`, error);
        return null;
    }
}

/**
 * 保存客户端配置
 * @param {string} clientId - 客户端ID
 * @param {object} configData - 配置数据
 * @returns {boolean} 是否保存成功
 */
function saveClientConfig(clientId, configData) {
    try {
        const clients = getClients();
        const alias = clients[clientId] || clientId;
        const clientDir = path.join(config.UPLOADS_DIR, alias);

        // 确保目录存在
        fs.mkdirSync(clientDir, { recursive: true });

        const configPath = path.join(clientDir, 'config.json');

        const fullConfig = {
            ...configData,
            lastUpload: getBeijingTime(),
            clientId: clientId,
            alias: alias
        };

        fs.writeFileSync(configPath, JSON.stringify(fullConfig, null, 2));
        logWithTime(`[CONFIG] Saved config to ${configPath}`);

        return true;
    } catch (error) {
        errorWithTime(`[CONFIG] Failed to save config for ${clientId}:`, error);
        return false;
    }
}

/**
 * 验证配置数据
 * @param {object} configData - 配置数据
 * @returns {object} 验证结果
 */
function validateConfigData(configData) {
    const errors = [];

    if (!configData || typeof configData !== 'object') {
        errors.push('Config data must be an object');
        return { valid: false, errors };
    }

    // 检查必需字段
    const requiredFields = ['server_url', 'ws_url'];
    for (const field of requiredFields) {
        if (!configData[field]) {
            errors.push(`Missing required field: ${field}`);
        }
    }

    // 检查数值字段
    const numericFields = ['interval_seconds', 'max_retries', 'retry_delay_ms'];
    for (const field of numericFields) {
        if (configData[field] !== undefined) {
            const value = Number(configData[field]);
            if (isNaN(value) || value < 0) {
                errors.push(`${field} must be a positive number`);
            }
        }
    }

    // 检查布尔字段
    if (configData.add_to_startup !== undefined && typeof configData.add_to_startup !== 'boolean') {
        errors.push('add_to_startup must be a boolean');
    }

    return {
        valid: errors.length === 0,
        errors
    };
}

/**
 * 处理配置更新请求
 * @param {string} clientId - 客户端ID
 * @param {object} configData - 新配置数据
 * @returns {object} 处理结果
 */
function handleConfigUpdate(clientId, configData) {
    // 验证客户端ID
    if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
        return {
            success: false,
            error: 'Missing or invalid client_id',
            status: 400
        };
    }

    // 验证配置数据
    const validation = validateConfigData(configData);
    if (!validation.valid) {
        return {
            success: false,
            error: 'Invalid config data: ' + validation.errors.join(', '),
            status: 400
        };
    }

    // 保存配置
    const saved = saveClientConfig(clientId, configData);
    if (!saved) {
        return {
            success: false,
            error: 'Failed to save config',
            status: 500
        };
    }

    return {
        success: true,
        message: 'Config saved successfully'
    };
}

module.exports = {
    getClientConfig,
    saveClientConfig,
    validateConfigData,
    handleConfigUpdate
};
