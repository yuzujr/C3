// services/client-service.js
// 新的客户端服务 - 基于数据库的客户端管理

const { Client, CommandLog, Screenshot, AuditLog } = require('../database/models');
const { logWithTime, errorWithTime } = require('../logger');
const cacheService = require('./cache-service');

class ClientService {
    constructor() {
        this.activeConnections = new Map(); // client_id -> websocket connection
        this.CACHE_KEYS = {
            ALL_CLIENTS: 'all_clients',
            CLIENT_BY_ID: 'client_by_id:',
            CLIENT_BY_ALIAS: 'client_by_alias:'
        };
    }

    /**
     * 注册或更新客户端
     * @param {string} clientId - 客户端ID
     * @param {object} clientInfo - 客户端信息
     */
    async registerClient(clientId, clientInfo = {}) {
        try {
            const { alias, hostname, ip_address, platform } = clientInfo;
            
            // 查找现有客户端
            let client = await Client.findOne({ where: { client_id: clientId } });
            
            if (client) {
                // 更新现有客户端信息
                await client.update({
                    alias: alias || client.alias,
                    hostname: hostname || client.hostname,
                    ip_address: ip_address || client.ip_address,
                    platform: platform || client.platform,
                    online_status: true,
                    last_seen: new Date()
                });
                
                logWithTime(`[CLIENT-SERVICE] Updated client: ${client.alias || clientId}`);
            } else {
                // 创建新客户端
                client = await Client.create({
                    client_id: clientId,
                    alias: alias || clientId,
                    hostname,
                    ip_address,
                    platform,
                    online_status: true,
                    last_seen: new Date()
                });
                
            }
            
            // 清除相关缓存
            this.clearClientCaches(clientId, client.alias);
            
            return client;
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to register client:', error);
            throw error;
        }
    }

    /**
     * 设置客户端离线状态
     * @param {string} clientId - 客户端ID
     */
    async setClientOffline(clientId) {
        try {
            const client = await Client.findOne({ where: { client_id: clientId } });
            if (client) {
                await client.update({
                    online_status: false,
                    last_seen: new Date()
                });
                
            }
            
            // 移除活跃连接
            this.activeConnections.delete(clientId);
            
            // 清除相关缓存
            this.clearClientCaches(clientId, client ? client.alias : null);
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to set client offline:', error);
        }
    }

    /**
     * 设置客户端在线状态
     * @param {string} clientId - 客户端ID
     */
    async setClientOnline(clientId) {
        try {
            const client = await Client.findOne({ where: { client_id: clientId } });
            if (client) {
                await client.update({
                    online_status: true,
                    last_seen: new Date()
                });
                
            }
            
            // 添加到活跃连接
            this.activeConnections.set(clientId, new Date());
            
            // 清除相关缓存
            this.clearClientCaches(clientId, client ? client.alias : null);
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to set client online:', error);
        }
    }

    /**
     * 获取所有客户端（带缓存）
     */
    async getAllClients() {
        try {
            // 先检查缓存
            const cached = cacheService.get(this.CACHE_KEYS.ALL_CLIENTS);
            if (cached) {
                return cached;
            }
            
            const clients = await Client.findAll({
                order: [['last_seen', 'DESC']]
            });
            
            const result = clients.map(client => ({
                id: client.id,
                client_id: client.client_id,
                alias: client.alias,
                hostname: client.hostname,
                ip_address: client.ip_address,
                platform: client.platform,
                online: client.online_status,
                last_seen: client.last_seen,
                created_at: client.created_at
            }));
            
            // 缓存结果，30秒过期
            cacheService.set(this.CACHE_KEYS.ALL_CLIENTS, result, 30000);
            
            return result;
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to get all clients:', error);
            return [];
        }
    }

    /**
     * 通过别名或ID获取客户端
     * @param {string} identifier - 客户端别名或ID
     */
    async getClientByIdentifier(identifier) {
        try {
            // 先按 client_id 查找
            let client = await Client.findOne({ where: { client_id: identifier } });
            
            // 如果没找到，再按 alias 查找
            if (!client) {
                client = await Client.findOne({ where: { alias: identifier } });
            }
            
            return client;
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to get client by identifier:', error);
            return null;
        }
    }

    /**
     * 更新客户端别名
     * @param {string} clientId - 客户端ID
     * @param {string} newAlias - 新别名
     */
    async updateClientAlias(clientId, newAlias) {
        try {
            const client = await Client.findOne({ where: { client_id: clientId } });
            if (client) {
                await client.update({ alias: newAlias });
                return true;
            }
            return false;
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to update client alias:', error);
            return false;
        }
    }

    /**
     * 记录命令执行
     * @param {string} clientId - 客户端ID
     * @param {string} command - 命令
     * @param {string} result - 执行结果
     * @param {number} exitCode - 退出码
     */
    async logCommand(clientId, command, result, exitCode) {
        try {
            await CommandLog.create({
                client_id: clientId,
                command,
                result,
                exit_code: exitCode,
                executed_at: new Date()
            });
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to log command:', error);
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
            await Screenshot.create({
                client_id: clientId,
                filename,
                file_path: filePath,
                file_size: fileSize,
                uploaded_at: new Date()
            });
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to log screenshot:', error);
        }
    }

    /**
     * 获取客户端命令历史
     * @param {string} clientId - 客户端ID
     * @param {number} limit - 限制条数
     */
    async getCommandHistory(clientId, limit = 50) {
        try {
            const commands = await CommandLog.findAll({
                where: { client_id: clientId },
                order: [['executed_at', 'DESC']],
                limit
            });
            
            return commands.map(cmd => ({
                id: cmd.id,
                command: cmd.command,
                result: cmd.result,
                exit_code: cmd.exit_code,
                executed_at: cmd.executed_at
            }));
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to get command history:', error);
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
            const screenshots = await Screenshot.findAll({
                where: { client_id: clientId },
                order: [['uploaded_at', 'DESC']],
                limit
            });
            
            return screenshots.map(screenshot => ({
                id: screenshot.id,
                filename: screenshot.filename,
                file_path: screenshot.file_path,
                file_size: screenshot.file_size,
                uploaded_at: screenshot.uploaded_at
            }));
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to get screenshot history:', error);
            return [];
        }
    }

    /**
     * 清除客户端相关缓存
     * @param {string} clientId - 客户端ID
     * @param {string} alias - 客户端别名
     */
    clearClientCaches(clientId, alias) {
        // 清除所有客户端列表缓存
        cacheService.delete(this.CACHE_KEYS.ALL_CLIENTS);
        
        // 清除特定客户端缓存
        if (clientId) {
            cacheService.delete(this.CACHE_KEYS.CLIENT_BY_ID + clientId);
        }
        if (alias) {
            cacheService.delete(this.CACHE_KEYS.CLIENT_BY_ALIAS + alias);
        }
    }

    /**
     * 清除所有缓存
     */
    clearAllCaches() {
        cacheService.clear();
    }

    /**
     * 设置活跃连接
     * @param {string} clientId - 客户端ID
     * @param {WebSocket} ws - WebSocket连接
     */
    setActiveConnection(clientId, ws) {
        this.activeConnections.set(clientId, ws);
    }

    /**
     * 获取活跃连接
     * @param {string} clientId - 客户端ID
     */
    getActiveConnection(clientId) {
        return this.activeConnections.get(clientId);
    }

    /**
     * 移除活跃连接
     * @param {string} clientId - 客户端ID
     */
    removeActiveConnection(clientId) {
        this.activeConnections.delete(clientId);
    }

    /**
     * 获取在线客户端数量
     */
    getOnlineCount() {
        return this.activeConnections.size;
    }

    /**
     * 删除客户端
     * @param {string} clientId - 客户端ID
     */
    async deleteClient(clientId) {
        try {
            const client = await Client.findOne({ where: { client_id: clientId } });
            if (!client) {
                return { success: false, message: 'Client not found' };
            }

            // 删除相关的命令日志
            await CommandLog.destroy({ where: { client_id: clientId } });
            
            // 删除相关的截图记录
            await Screenshot.destroy({ where: { client_id: clientId } });
            
            // 删除审计日志
            await AuditLog.destroy({ where: { client_id: clientId } });
            
            // 删除客户端记录
            await client.destroy();
            
            // 清理缓存
            this.clearCacheForClient(clientId, client.alias);
            
            // 移除活动连接
            this.activeConnections.delete(clientId);
            
            logWithTime(`[CLIENT-SERVICE] Deleted client: ${client.alias} (${clientId})`);
            return { success: true, message: 'Client deleted successfully' };
        } catch (error) {
            errorWithTime('[CLIENT-SERVICE] Failed to delete client:', error);
            return { success: false, message: 'Failed to delete client' };
        }
    }

    /**
     * 获取所有在线客户端ID
     */
    getOnlineClientIds() {
        return Array.from(this.activeConnections.keys());
    }
}

module.exports = new ClientService();
