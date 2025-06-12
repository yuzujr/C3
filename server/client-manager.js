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
    }

    /**
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
            logWithTime('[CLIENT-MANAGER] Loaded clients:', Object.keys(this.clientsMapping).length);
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
                logWithTime('[CLIENT-MANAGER] Clients file changed, reloading...');
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
        for (const [clientId, clientAlias] of Object.entries(this.clientsMapping)) {
            if (clientAlias === alias) {
                return clientId;
            }
        }
        // 如果没找到映射，可能alias就是clientId
        return this.clientsMapping[alias] !== undefined ? alias : null;
    }

    /**
     * 根据alias获取client_id (别名方法)
     * @param {string} alias - 客户端别名
     * @returns {string|null} client_id
     */
    getClientIdByAlias(alias) {
        return this.getClientId(alias);
    }

    /**
     * 根据client_id获取alias (别名方法)
     * @param {string} clientId - 客户端ID
     * @returns {string} alias
     */
    getAliasByClientId(clientId) {
        return this.getAlias(clientId);
    }    /**
     * 获取所有客户端信息
     * @returns {Array} 客户端信息数组
     */
    getAllClients() {
        return Object.entries(this.clientsMapping).map(([clientId, alias]) => ({
            alias: alias,
            online: false // 默认离线，需要WebSocket模块提供在线状态
        }));
    }

    /**
     * 添加或更新客户端
     * @param {string} clientId - 客户端ID
     * @param {string} alias - 客户端别名
     */
    setClient(clientId, alias) {
        this.clientsMapping[clientId] = alias;
        this.saveClients();
    }

    /**
     * 保存客户端映射到文件
     */
    saveClients() {
        try {
            fs.writeFileSync(this.clientsJsonPath, JSON.stringify(this.clientsMapping, null, 2));
            logWithTime('[CLIENT-MANAGER] Clients saved to disk');
        } catch (err) {
            errorWithTime('[CLIENT-MANAGER] Failed to save clients:', err.message);
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
        }
        // 如果是alias，直接返回
        return identifier;
    }
}

// 单例模式
const clientManager = new ClientManager();

module.exports = clientManager;
