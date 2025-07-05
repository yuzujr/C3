// services/cache-service.js
// 内存缓存服务，用于减少数据库查询

const { logWithTime } = require('../logger');

class CacheService {
    constructor() {
        this.cache = new Map();
        this.cacheExpiry = new Map();
    }

    /**
     * 设置缓存
     * @param {string} key - 缓存键
     * @param {*} value - 缓存值
     * @param {number} ttl - 过期时间(毫秒)，默认30秒
     */
    set(key, value, ttl = 30000) {
        this.cache.set(key, value);
        this.cacheExpiry.set(key, Date.now() + ttl);
        
        // 设置定时清理
        setTimeout(() => {
            this.delete(key);
        }, ttl);
    }

    /**
     * 获取缓存
     * @param {string} key - 缓存键
     * @returns {*} 缓存值或undefined
     */
    get(key) {
        const expiry = this.cacheExpiry.get(key);
        
        if (!expiry || Date.now() > expiry) {
            // 缓存已过期
            this.delete(key);
            return undefined;
        }
        
        return this.cache.get(key);
    }

    /**
     * 删除缓存
     * @param {string} key - 缓存键
     */
    delete(key) {
        this.cache.delete(key);
        this.cacheExpiry.delete(key);
    }

    /**
     * 清空所有缓存
     */
    clear() {
        this.cache.clear();
        this.cacheExpiry.clear();
    }

    /**
     * 获取缓存统计信息
     */
    getStats() {
        return {
            size: this.cache.size,
            keys: Array.from(this.cache.keys())
        };
    }

    /**
     * 清理过期缓存
     */
    cleanExpired() {
        const now = Date.now();
        for (const [key, expiry] of this.cacheExpiry.entries()) {
            if (now > expiry) {
                this.delete(key);
            }
        }
    }
}

// 创建全局缓存实例
const cacheService = new CacheService();

// 每分钟清理一次过期缓存
setInterval(() => {
    cacheService.cleanExpired();
    logWithTime(`[CACHE] Cleaned expired cache entries. Current size: ${cacheService.cache.size}`);
}, 60000);

module.exports = cacheService;
