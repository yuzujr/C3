// services/auth-service.js
// 新的认证服务 - 基于数据库的用户认证

const bcrypt = require('bcryptjs');
const crypto = require('crypto');
const { User, UserSession } = require('../database/models');
const { logWithTime, errorWithTime } = require('../logger');
const cacheService = require('./cache-service');

class AuthService {
    constructor() {
        this.sessionExpireHours = 24; // 24小时过期
        this.cleanupInterval = 60 * 60 * 1000; // 1小时清理一次过期会话
        this.sessionCacheTime = 5 * 60 * 1000; // 会话缓存5分钟
        this.CACHE_KEYS = {
            SESSION: 'session:',
            USER: 'user:'
        };
        this.startCleanupTimer();
    }

    /**
     * 用户登录验证
     * @param {string} username - 用户名
     * @param {string} password - 密码
     * @returns {object|null} 登录结果
     */
    async login(username, password) {
        try {
            // 查找用户
            const user = await User.findOne({ where: { username } });
            if (!user) {
                return { success: false, message: '用户名或密码错误' };
            }

            // 验证密码
            const isPasswordValid = await bcrypt.compare(password, user.password_hash);
            if (!isPasswordValid) {
                return { success: false, message: '用户名或密码错误' };
            }

            // 创建会话
            const sessionId = this.generateSessionId();
            const expiresAt = new Date(Date.now() + (this.sessionExpireHours * 60 * 60 * 1000));

            await UserSession.create({
                session_id: sessionId,
                user_id: user.id,
                expires_at: expiresAt
            });


            return {
                success: true,
                sessionId,
                user: {
                    id: user.id,
                    username: user.username,
                    role: user.role
                }
            };
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Login failed:', error);
            return { success: false, message: '登录失败' };
        }
    }

    /**
     * 验证会话
     * @param {string} sessionId - 会话ID
     * @returns {object|null} 验证结果
     */
    async validateSession(sessionId) {
        try {
            if (!sessionId) {
                return null;
            }

            // 先从缓存中获取会话
            const cacheKey = this.CACHE_KEYS.SESSION + sessionId;
            const cachedSession = cacheService.get(cacheKey);
            if (cachedSession) {
                // 检查缓存的会话是否过期
                if (new Date(cachedSession.expires_at) > new Date()) {
                    return cachedSession;
                }
                // 缓存过期，删除缓存
                cacheService.delete(cacheKey);
            }

            const session = await UserSession.findOne({
                where: { session_id: sessionId },
                include: [{ model: User }]
            });

            if (!session) {
                return null;
            }

            // 检查会话是否过期
            if (session.expires_at < new Date()) {
                await session.destroy();
                return null;
            }

            // 只有距离上次更新超过1分钟才更新数据库，减少写入频率
            const now = new Date();
            const lastAccess = new Date(session.last_access);
            const timeDiff = now - lastAccess;
            
            if (timeDiff > 60000) { // 1分钟
                await session.update({ last_access: now });
            }

            const result = {
                user: {
                    id: session.User.id,
                    username: session.User.username,
                    role: session.User.role
                },
                sessionId: session.session_id,
                expires_at: session.expires_at
            };

            // 缓存会话信息（5分钟）
            cacheService.set(cacheKey, result, this.sessionCacheTime);

            return result;
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Session validation failed:', error);
            return null;
        }
    }

    /**
     * 用户登出
     * @param {string} sessionId - 会话ID
     */
    async logout(sessionId) {
        try {
            if (sessionId) {
                // 清除缓存
                cacheService.delete(this.CACHE_KEYS.SESSION + sessionId);
                
                // 删除数据库中的会话
                await UserSession.destroy({
                    where: { session_id: sessionId }
                });
            }
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Logout failed:', error);
        }
    }

    /**
     * 创建新用户
     * @param {string} username - 用户名
     * @param {string} password - 密码
     * @param {string} role - 角色
     */
    async createUser(username, password, role = 'user') {
        try {
            // 检查用户名是否已存在
            const existingUser = await User.findOne({ where: { username } });
            if (existingUser) {
                return { success: false, message: '用户名已存在' };
            }

            // 加密密码
            const saltRounds = 10;
            const password_hash = await bcrypt.hash(password, saltRounds);

            // 创建用户
            const user = await User.create({
                username,
                password_hash,
                role
            });

            logWithTime(`[AUTH-SERVICE] Created new user: ${username}`);

            return {
                success: true,
                user: {
                    id: user.id,
                    username: user.username,
                    role: user.role
                }
            };
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Failed to create user:', error);
            return { success: false, message: '创建用户失败' };
        }
    }

    /**
     * 更新用户密码
     * @param {number} userId - 用户ID
     * @param {string} oldPassword - 旧密码
     * @param {string} newPassword - 新密码
     */
    async updatePassword(userId, oldPassword, newPassword) {
        try {
            const user = await User.findByPk(userId);
            if (!user) {
                return { success: false, message: '用户不存在' };
            }

            // 验证旧密码
            const isOldPasswordValid = await bcrypt.compare(oldPassword, user.password_hash);
            if (!isOldPasswordValid) {
                return { success: false, message: '旧密码错误' };
            }

            // 加密新密码
            const saltRounds = 10;
            const password_hash = await bcrypt.hash(newPassword, saltRounds);

            // 更新密码
            await user.update({ password_hash });

            logWithTime(`[AUTH-SERVICE] Password updated for user: ${user.username}`);

            return { success: true, message: '密码更新成功' };
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Failed to update password:', error);
            return { success: false, message: '密码更新失败' };
        }
    }

    /**
     * 获取所有用户
     */
    async getAllUsers() {
        try {
            const users = await User.findAll({
                attributes: ['id', 'username', 'role', 'created_at', 'updated_at'],
                order: [['created_at', 'DESC']]
            });

            return users.map(user => ({
                id: user.id,
                username: user.username,
                role: user.role,
                created_at: user.created_at,
                updated_at: user.updated_at
            }));
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Failed to get all users:', error);
            return [];
        }
    }

    /**
     * 生成会话ID
     * @returns {string} 会话ID
     */
    generateSessionId() {
        return crypto.randomBytes(32).toString('hex');
    }

    /**
     * 启动清理过期会话的定时器
     */
    startCleanupTimer() {
        setInterval(async () => {
            try {
                const deletedCount = await UserSession.destroy({
                    where: {
                        expires_at: {
                            [require('sequelize').Op.lt]: new Date()
                        }
                    }
                });

                if (deletedCount > 0) {
                    logWithTime(`[AUTH-SERVICE] Cleaned up ${deletedCount} expired sessions`);
                }
            } catch (error) {
                errorWithTime('[AUTH-SERVICE] Failed to cleanup expired sessions:', error);
            }
        }, this.cleanupInterval);
    }

    /**
     * 获取活跃会话数量
     */
    async getActiveSessionCount() {
        try {
            const count = await UserSession.count({
                where: {
                    expires_at: {
                        [require('sequelize').Op.gt]: new Date()
                    }
                }
            });
            return count;
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Failed to get active session count:', error);
            return 0;
        }
    }
}

module.exports = new AuthService();
