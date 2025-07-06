// 命令服务模块
// 处理客户端命令相关的业务逻辑

const fs = require('fs');
const path = require('path');
const config = require('../config');
const { errorWithTime, getBeijingTime } = require('../logger');

/**
 * 获取客户端配置
 * @param {string} identifier - 客户端ID或别名
 * @returns {Promise<object|null>} 配置对象或null
 */
async function getClientConfig(identifier) {
    try {
        // 使用 client_id 作为目录路径
        const configPath = path.join(config.UPLOADS_DIR, identifier, 'config.json');

        if (!fs.existsSync(configPath)) {
            return null;
        }

        const raw = fs.readFileSync(configPath, 'utf-8').trim();
        const configData = raw ? JSON.parse(raw) : {};
        return configData || null;
    } catch (error) {
        errorWithTime(`[CLIENT] Failed to read config for ${identifier}:`, error);
        return null;
    }
}

/**
 * 保存客户端配置
 * @param {string} clientId - 客户端ID
 * @param {object} configData - 配置数据
 * @returns {Promise<boolean>} 是否保存成功
 */
async function saveClientConfig(clientId, configData) {
    try {
        const clientDir = path.join(config.UPLOADS_DIR, clientId);

        // 确保目录存在
        fs.mkdirSync(clientDir, { recursive: true });

        const configPath = path.join(clientDir, 'config.json');

        // 检查是否有嵌套的 'api' 对象
        let apiConfig;
        if (configData.api && typeof configData.api === 'object') {
            // 如果配置数据包含 api 对象，使用它
            apiConfig = configData.api;
        } else {
            // 如果配置数据在根级别，包装在 api 对象中
            apiConfig = configData;
        }

        const fullConfig = {
            api: {
                ...apiConfig,
                client_id: clientId  // 确保 client_id 正确设置
            },
            lastUpload: getBeijingTime(),
            clientId: clientId,
            // 如果原始数据包含构建信息，保留它
            ...(configData.build_info && { build_info: configData.build_info })
        };

        fs.writeFileSync(configPath, JSON.stringify(fullConfig, null, 2));

        return true;
    } catch (error) {
        errorWithTime(`[CLIENT] Failed to save config for ${clientId}:`, error);
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

    // 严格要求必须包含 'api' 字段
    if (!configData.api || typeof configData.api !== 'object') {
        errors.push('Config data must contain an "api" object');
        return { valid: false, errors };
    }

    const config = configData.api;    // 检查必需字段
    const requiredFields = ['hostname', 'port'];
    for (const field of requiredFields) {
        if (!config[field]) {
            errors.push(`Missing required field: ${field}`);
        }
    }

    // 检查主机名
    if (config.hostname && typeof config.hostname !== 'string') {
        errors.push('hostname must be a string');
    }

    // 检查端口
    if (config.port !== undefined) {
        const port = Number(config.port);
        if (isNaN(port) || port < 1 || port > 65535) {
            errors.push('port must be a number between 1 and 65535');
        }
    }

    // 检查数值字段
    const numericFields = ['interval_seconds', 'max_retries', 'retry_delay_ms'];
    for (const field of numericFields) {
        if (config[field] !== undefined) {
            const value = Number(config[field]);
            if (isNaN(value) || value < 0) {
                errors.push(`${field} must be a positive number`);
            }
        }
    }

    // 检查布尔字段
    if (config.add_to_startup !== undefined && typeof config.add_to_startup !== 'boolean') {
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
 * @returns {Promise<object>} 处理结果
 */
async function handleConfigUpdate(clientId, configData) {
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
    const saved = await saveClientConfig(clientId, configData);
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
