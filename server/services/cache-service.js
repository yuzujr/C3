// services/cache-service.js
// 简化的内存缓存服务

const { logWithTime } = require('../logger');

class CacheService {
    constructor() {
        this.cache = new Map();
        this.cacheExpiry = new Map();
        this.startCleanupTimer();
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
    }

    /**
     * 获取缓存
     * @param {string} key - 缓存键
     */
    get(key) {
        const expiry = this.cacheExpiry.get(key);
        
        if (!expiry || Date.now() > expiry) {
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
     * 获取缓存大小
     */
    get size() {
        return this.cache.size;
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

    /**
     * 启动定期清理任务
     */
    startCleanupTimer() {
        // 每分钟清理一次过期缓存
        setInterval(() => {
            this.cleanExpired();
            if (this.cache.size > 0) {
                logWithTime(`[CACHE] Cleaned expired entries. Current size: ${this.cache.size}`);
            }
        }, 60000);
    }
}

module.exports = new CacheService();
