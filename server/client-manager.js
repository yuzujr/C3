// client-manager.js
// 统一的客户端管理器 (使用数据库)
// 负责处理客户端标识符的统一管理和转换

const clientService = require('./services/client-service');
const { logWithTime, errorWithTime } = require('./logger');

class ClientManager {
    constructor() {
        this.initializeFromDatabase();
    }

    /**
     * 从数据库初始化客户端信息
     */
    async initializeFromDatabase() {
        try {
            const clients = await clientService.getAllClients();
            const onlineCount = clients.filter(c => c.online).length;
            const totalCount = clients.length;
            
            logWithTime(`[CLIENT-MANAGER] Loaded ${totalCount} clients from database`);
            
            if (totalCount > 0) {
                logWithTime('[CLIENT-MANAGER] Registered clients:');
                clients.forEach(client => {
                    const status = client.online ? '✓ online' : '○ offline';
                    logWithTime(`[CLIENT-MANAGER]  - ${client.alias} (${client.client_id})`);
                });
            }
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to initialize from database:', error);
        }
    }

    /**
     * 通过client_id获取alias
     * @param {string} clientId - 客户端ID
     * @returns {Promise<string>} alias
     */
    async getAlias(clientId) {
        try {
            const client = await clientService.getClientByIdentifier(clientId);
            return client ? client.alias : clientId;
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to get alias:', error);
            return clientId;
        }
    }

    /**
     * 获取所有客户端信息
     * @returns {Promise<Array>} 客户端信息数组
     */
    async getAllClients() {
        try {
            return await clientService.getAllClients();
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to get all clients:', error);
            return [];
        }
    }

    /**
     * 添加或更新客户端
     * @param {string} clientId - 客户端ID
     * @param {string} alias - 客户端别名
     * @param {object} clientInfo - 客户端信息
     */
    async setClient(clientId, alias, clientInfo = {}) {
        try {
            await clientService.registerClient(clientId, {
                alias,
                ...clientInfo
            });
            logWithTime(`[CLIENT-MANAGER] Client registered: ${alias} (${clientId})`);
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to set client:', error);
        }
    }

    /**
     * 更新客户端别名
     * @param {string} clientId - 客户端ID
     * @param {string} newAlias - 新别名
     */
    async updateClientAlias(clientId, newAlias) {
        try {
            const success = await clientService.updateClientAlias(clientId, newAlias);
            if (success) {
                logWithTime(`[CLIENT-MANAGER] Client alias updated: ${clientId} -> ${newAlias}`);
            }
            return success;
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to update client alias:', error);
            return false;
        }
    }

    /**
     * 设置客户端在线状态
     * @param {string} clientId - 客户端ID
     * @param {boolean} online - 在线状态
     */
    async setClientOnlineStatus(clientId, online) {
        try {
            if (online) {
                await clientService.setClientOnline(clientId);
            } else {
                await clientService.setClientOffline(clientId);
            }
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to set client online status:', error);
        }
    }

    /**
     * 删除客户端 (标记为离线，不真正删除数据库记录)
     * @param {string} clientId - 客户端ID
     */
    async removeClient(clientId) {
        try {
            await clientService.setClientOffline(clientId);
            logWithTime(`[CLIENT-MANAGER] Client marked offline: ${clientId}`);
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to remove client:', error);
        }
    }

    /**
     * 删除客户端
     * @param {string} clientId - 客户端ID
     */
    async deleteClientByClientId(clientId) {
        try {
            const client = await clientService.getClientByIdentifier(clientId);
            if (!client) {
                return { success: false, message: 'Client not found' };
            }

            const clientId = client.client_id;
            
            // 删除客户端记录
            const result = await clientService.deleteClient(clientId);
            
            if (result.success) {
                // 删除客户端上传目录
                await this.deleteClientDirectory(clientId);
                
                logWithTime(`[CLIENT-MANAGER] Client deleted: ${client.alias} (${clientId})`);
                return { success: true, message: 'Client deleted successfully' };
            } else {
                return result;
            }
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to delete client:', error);
            return { success: false, message: 'Failed to delete client' };
        }
    }

    /**
     * 删除客户端目录
     * @param {string} clientId - 客户端ID
     */
    async deleteClientDirectory(clientId) {
        const fs = require('fs');
        const path = require('path');
        
        try {
            const config = require('./config');
            const clientDir = path.join(config.UPLOADS_DIR, clientId);
            
            if (fs.existsSync(clientDir)) {
                // 递归删除目录
                fs.rmSync(clientDir, { recursive: true, force: true });
                logWithTime(`[CLIENT-MANAGER] Deleted client directory: ${clientDir}`);
            }
        } catch (error) {
            errorWithTime(`[CLIENT-MANAGER] Failed to delete client directory for ${clientId}:`, error);
        }
    }

    /**
     * 根据客户端ID获取客户端信息
     * @param {string} clientId - 客户端ID
     * @returns {Promise<object|null>} 客户端信息
     */
    async getClientByClientId(clientId) {
        try {
            return await clientService.getClientByIdentifier(clientId);
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to get client by client ID:', error);
            return null;
        }
    }

    /**
     * 检查客户端是否存在
     * @param {string} clientId - 客户端ID
     * @returns {Promise<boolean>} 是否存在
     */
    async hasClient(clientId) {
        try {
            const client = await clientService.getClientByIdentifier(clientId);
            return !!client;
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to check client existence:', error);
            return false;
        }
    }

    /**
     * 确保上传目录存在
     * @param {string} clientId - 客户端ID
     * @returns {Promise<string>} 客户端上传目录路径
     */
    async ensureClientUploadDir(clientId) {
        const fs = require('fs');
        const path = require('path');
        
        try {
            const config = require('./config');
            const clientDir = path.join(config.UPLOADS_DIR, clientId);
            
            if (!fs.existsSync(clientDir)) {
                fs.mkdirSync(clientDir, { recursive: true });
                logWithTime(`[CLIENT-MANAGER] Created upload directory: ${clientDir}`);
            }
            
            return clientDir;
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to ensure client upload directory:', error);
            throw error;
        }
    }

    /**
     * 记录命令执行
     * @param {string} clientId - 客户端ID
     * @param {string} command - 命令
     * @param {string} result - 结果
     * @param {number} exitCode - 退出码
     */
    async logCommand(clientId, command, result, exitCode) {
        try {
            await clientService.logCommand(clientId, command, result, exitCode);
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to log command:', error);
        }
    }

    /**
     * 记录截图上传
     * @param {string} clientId - 客户端ID
     * @param {string} filename - 文件名
     * @param {string} filePath - 文件路径
     * @param {number} fileSize - 文件大小
     */
    async logScreenshot(clientId, filename, filePath, fileSize) {
        try {
            await clientService.logScreenshot(clientId, filename, filePath, fileSize);
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to log screenshot:', error);
        }
    }

    /**
     * 获取客户端命令历史
     * @param {string} clientId - 客户端ID
     * @param {number} limit - 限制条数
     */
    async getCommandHistory(clientId, limit = 50) {
        try {
            return await clientService.getCommandHistory(clientId, limit);
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to get command history:', error);
            return [];
        }
    }

    /**
     * 获取客户端截图历史
     * @param {string} clientId - 客户端ID
     * @param {number} limit - 限制条数
     */
    async getScreenshotHistory(clientId, limit = 20) {
        try {
            return await clientService.getScreenshotHistory(clientId, limit);
        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to get screenshot history:', error);
            return [];
        }
    }

}

// 导出单例实例
module.exports = new ClientManager();
