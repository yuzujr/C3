// client-manager.js
// 统一的客户端管理器
// 负责处理客户端标识符的统一管理和转换

const fs = require('fs');
const path = require('path');
const { logWithTime, errorWithTime } = require('./logger');

class ClientManager {
    constructor() {
        this.clientsJsonPath = path.join(__dirname, 'clients.json');
        this.uploadsDir = path.join(__dirname, 'uploads');
        this.clientsMapping = {}; // client_id -> alias 映射
        this.loadClients();

        // 监听文件变化
        this.watchClientsFile();
    }    /**
     * 加载客户端映射
     */
    loadClients() {
        try {
            if (fs.existsSync(this.clientsJsonPath)) {
                const data = fs.readFileSync(this.clientsJsonPath, 'utf-8').trim();
                this.clientsMapping = data ? JSON.parse(data) : {};
            } else {
                this.clientsMapping = {};
            }

            const clientCount = Object.keys(this.clientsMapping).length;

            if (clientCount > 0) {
                logWithTime('[CLIENT-MANAGER] Registered clients:');
                Object.entries(this.clientsMapping).forEach(([clientId, alias]) => {
                    logWithTime(`[CLIENT-MANAGER]  - ${alias} (${clientId})`);
                });
            }
        } catch (err) {
            errorWithTime('[CLIENT-MANAGER] Failed to load clients:', err.message);
            this.clientsMapping = {};
        }
    }

    /**
     * 监听clients.json文件变化
     */
    watchClientsFile() {
        if (fs.existsSync(this.clientsJsonPath)) {
            fs.watchFile(this.clientsJsonPath, () => {
                this.loadClients();
            });
        }
    }

    /**
     * 通过client_id获取alias
     * @param {string} clientId - 客户端ID
     * @returns {string} alias
     */
    getAlias(clientId) {
        return this.clientsMapping[clientId] || clientId;
    }    /**
     * 通过alias获取client_id
     * @param {string} alias - 客户端别名
     * @returns {string|null} client_id
     */
    getClientId(alias) {
        // 情况1：直接是clientId（在映射中作为key存在）
        if (this.clientsMapping.hasOwnProperty(alias)) {
            return alias;
        }

        // 情况2：是别名，需要查找对应的clientId
        for (const [clientId, clientAlias] of Object.entries(this.clientsMapping)) {
            if (clientAlias === alias) {
                return clientId;
            }
        }

        // 情况3：都没找到，返回null
        return null;
    }    /**
     * 获取所有客户端信息
     * @returns {Array} 客户端信息数组
     */
    getAllClients() {
        return Object.entries(this.clientsMapping).map(([clientId, alias]) => ({
            alias: alias,
            online: false // 默认离线，需要WebSocket模块提供在线状态
        }));
    }/**
     * 添加或更新客户端
     * @param {string} clientId - 客户端ID
     * @param {string} alias - 客户端别名
     */
    setClient(clientId, alias) {
        this.clientsMapping[clientId] = alias;
        this.saveClients();
    }

    /**
     * 更新客户端别名
     * @param {string} oldAlias - 旧的别名
     * @param {string} newAlias - 新的别名
     * @returns {Promise<{success: boolean, message: string}>}
     */
    async updateAlias(oldAlias, newAlias) {
        try {
            // 验证输入
            if (!oldAlias || !newAlias) {
                return { success: false, message: '别名不能为空' };
            }

            if (oldAlias === newAlias) {
                return { success: false, message: '新别名与当前别名相同' };
            }

            // 检查新别名是否已存在
            const existingClientId = this.getClientId(newAlias);
            if (existingClientId) {
                return { success: false, message: '新别名已被其他客户端使用' };
            }

            // 找到对应的clientId
            const clientId = this.getClientId(oldAlias);
            if (!clientId) {
                return { success: false, message: '找不到对应的客户端' };
            }

            // 更新映射
            this.clientsMapping[clientId] = newAlias;
            this.saveClients();

            // 重命名uploads文件夹
            const oldDir = path.join(this.uploadsDir, oldAlias);
            const newDir = path.join(this.uploadsDir, newAlias);

            if (fs.existsSync(oldDir)) {
                if (fs.existsSync(newDir)) {
                    return { success: false, message: '新别名对应的文件夹已存在' };
                }

                try {
                    fs.renameSync(oldDir, newDir);
                } catch (renameError) {
                    // 如果重命名失败，回滚映射更改
                    this.clientsMapping[clientId] = oldAlias;
                    this.saveClients();
                    return { success: false, message: '重命名文件夹失败: ' + renameError.message };
                }
            }
            logWithTime(`[CLIENT-MANAGER] Alias updated: ${oldAlias} -> ${newAlias}`);

            // 通知WebSocket模块更新别名映射
            try {
                const { updateClientAlias } = require('./websocket');
                updateClientAlias(oldAlias, newAlias);
            } catch (error) {
                errorWithTime('[CLIENT-MANAGER] Failed to notify WebSocket about alias update:', error.message);
                // 不影响主要功能，继续执行
            }

            return { success: true, message: '别名更新成功' };

        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to update alias:', error.message);
            return { success: false, message: '更新别名时发生错误: ' + error.message };
        }
    }

    /**
     * 删除客户端
     * @param {string} alias - 客户端别名
     * @returns {Promise<{success: boolean, message: string}>}
     */
    async deleteClient(alias) {
        try {
            const clientId = this.getClientId(alias);
            if (!clientId) {
                return { success: false, message: '找不到对应的客户端' };
            }

            // 删除映射
            delete this.clientsMapping[clientId];
            this.saveClients();

            // 可选：删除uploads文件夹（谨慎操作）
            const clientDir = path.join(this.uploadsDir, alias);
            if (fs.existsSync(clientDir)) {
                try {
                    fs.rmSync(clientDir, { recursive: true, force: true });
                    logWithTime(`[CLIENT-MANAGER] Deleted directory: ${alias}`);
                } catch (deleteError) {
                    errorWithTime('[CLIENT-MANAGER] Failed to delete directory:', deleteError.message);
                    // 继续执行，不阻止客户端删除
                }
            } logWithTime(`[CLIENT-MANAGER] Deleted client: ${alias} (clientId: ${clientId})`);

            // 通知WebSocket模块删除客户端连接
            try {
                const { removeClientConnection } = require('./websocket');
                removeClientConnection(alias);
            } catch (error) {
                errorWithTime('[CLIENT-MANAGER] Failed to notify WebSocket about client deletion:', error.message);
                // 不影响主要功能，继续执行
            }

            return { success: true, message: '客户端删除成功' };

        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Failed to delete client:', error.message);
            return { success: false, message: '删除客户端时发生错误: ' + error.message };
        }
    }

    /**
     * 保存客户端映射到文件
     */
    saveClients() {
        try {
            fs.writeFileSync(this.clientsJsonPath, JSON.stringify(this.clientsMapping, null, 2));
        } catch (err) {
        }
    }

    /**
     * 检查alias是否对应实际的上传目录
     * @param {string} alias - 客户端别名
     * @returns {boolean}
     */
    hasUploadsDir(alias) {
        const clientDir = path.join(this.uploadsDir, alias);
        return fs.existsSync(clientDir) && fs.statSync(clientDir).isDirectory();
    }

    /**
     * 规范化客户端标识符
     * 统一使用alias作为外部API的标识符
     * @param {string} identifier - 可能是clientId或alias
     * @returns {string} 规范化的alias
     */
    normalizeIdentifier(identifier) {
        // 如果是clientId，返回对应的alias
        if (this.clientsMapping[identifier]) {
            return this.clientsMapping[identifier];
        }        // 如果是alias，直接返回
        return identifier;
    }

    /**
     * 清理资源（在服务器关闭时调用）
     */    cleanup() {
        try {
            // 停止文件监听
            if (fs.existsSync(this.clientsJsonPath)) {
                fs.unwatchFile(this.clientsJsonPath);
            }

            // 清空内存中的映射
            this.clientsMapping = {};

        } catch (error) {
            errorWithTime('[CLIENT-MANAGER] Error during cleanup:', error);
        }
    }
}

// 单例模式
const clientManager = new ClientManager();

module.exports = clientManager;
