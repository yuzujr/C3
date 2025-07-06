// services/auth-service.js
// 简化的认证服务

const bcrypt = require('bcryptjs');
const crypto = require('crypto');
const { User, UserSession } = require('../database/models');
const { logWithTime, errorWithTime } = require('../logger');
const cacheService = require('./cache-service');

class AuthService {
    constructor() {
        this.sessionExpireHours = 24;
        this.sessionCacheTime = 5 * 60 * 1000; // 5分钟
        this.startCleanupTimer();
    }

    /**
     * 用户登录
     * @param {string} username - 用户名
     * @param {string} password - 密码
     */
    async login(username, password) {
        try {
            const user = await User.findOne({ where: { username } });
            if (!user || !await bcrypt.compare(password, user.password_hash)) {
                return { success: false, message: '用户名或密码错误' };
            }

            // 创建会话
            const sessionId = crypto.randomBytes(32).toString('hex');
            const expiresAt = new Date(Date.now() + (this.sessionExpireHours * 60 * 60 * 1000));

            await UserSession.create({
                session_id: sessionId,
                user_id: user.id,
                expires_at: expiresAt
            });

            const userInfo = {
                id: user.id,
                username: user.username,
                role: user.role
            };

            // 缓存会话信息
            cacheService.set(`session:${sessionId}`, {
                sessionId,
                user: userInfo,
                expires_at: expiresAt
            }, this.sessionCacheTime);

            return { success: true, sessionId, user: userInfo };
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Login failed:', error);
            return { success: false, message: '登录失败' };
        }
    }

    /**
     * 验证会话
     * @param {string} sessionId - 会话ID
     */
    async validateSession(sessionId) {
        try {
            if (!sessionId) return null;

            // 检查缓存
            const cached = cacheService.get(`session:${sessionId}`);
            if (cached && new Date(cached.expires_at) > new Date()) {
                return cached;
            }

            // 查询数据库
            const session = await UserSession.findOne({
                where: { session_id: sessionId },
                include: [{ model: User }]
            });

            if (!session || new Date(session.expires_at) <= new Date()) {
                if (session) await session.destroy();
                return null;
            }

            // 更新最后访问时间
            await session.update({ last_access: new Date() });

            const result = {
                sessionId: session.session_id,
                user: {
                    id: session.User.id,
                    username: session.User.username,
                    role: session.User.role
                },
                expires_at: session.expires_at
            };

            // 缓存结果
            cacheService.set(`session:${sessionId}`, result, this.sessionCacheTime);

            return result;
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Session validation failed:', error);
            return null;
        }
    }

    /**
     * 注销会话
     * @param {string} sessionId - 会话ID
     */
    async logout(sessionId) {
        try {
            if (!sessionId) return;

            // 删除数据库中的会话
            await UserSession.destroy({ where: { session_id: sessionId } });
            
            // 删除缓存
            cacheService.delete(`session:${sessionId}`);
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Logout failed:', error);
        }
    }

    /**
     * 清理过期会话
     */
    async cleanupExpiredSessions() {
        try {
            const { Op } = require('sequelize');
            const deleted = await UserSession.destroy({
                where: {
                    expires_at: {
                        [Op.lt]: new Date()
                    }
                }
            });
            
            if (deleted > 0) {
                logWithTime(`[AUTH-SERVICE] Cleaned up ${deleted} expired sessions`);
            }
        } catch (error) {
            errorWithTime('[AUTH-SERVICE] Failed to cleanup expired sessions:', error);
        }
    }

    /**
     * 启动定期清理任务
     */
    startCleanupTimer() {
        // 每小时清理一次过期会话
        setInterval(() => {
            this.cleanupExpiredSessions();
        }, 60 * 60 * 1000);
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
