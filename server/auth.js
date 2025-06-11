// 认证模块
// 处理用户登录验证和会话管理

const crypto = require('crypto');
const config = require('./config');
const { logWithTime, errorWithTime } = require('./logger');

// 存储活跃会话 (生产环境应使用Redis等)
const activeSessions = new Map();

/**
 * 生成随机会话ID
 */
function generateSessionId() {
    return crypto.randomBytes(32).toString('hex');
}

/**
 * 验证用户名和密码
 * @param {string} username - 用户名
 * @param {string} password - 密码
 * @returns {boolean} 验证结果
 */
function validateCredentials(username, password) {
    return username === config.AUTH_USERNAME && password === config.AUTH_PASSWORD;
}

/**
 * 创建新会话
 * @param {string} username - 用户名
 * @returns {string} 会话ID
 */
function createSession(username) {
    const sessionId = generateSessionId();
    const expiresAt = Date.now() + (config.SESSION_EXPIRE_HOURS * 60 * 60 * 1000);

    activeSessions.set(sessionId, {
        username,
        createdAt: Date.now(),
        expiresAt,
        lastAccess: Date.now()
    });

    logWithTime(`[AUTH] New session created for user: ${username}`);
    return sessionId;
}

/**
 * 验证会话
 * @param {string} sessionId - 会话ID
 * @returns {object|null} 会话信息或null
 */
function validateSession(sessionId) {
    if (!sessionId || !activeSessions.has(sessionId)) {
        return null;
    }

    const session = activeSessions.get(sessionId);

    // 检查会话是否过期
    if (Date.now() > session.expiresAt) {
        activeSessions.delete(sessionId);
        logWithTime(`[AUTH] Session expired: ${sessionId.substring(0, 8)}...`);
        return null;
    }

    // 更新最后访问时间
    session.lastAccess = Date.now();
    return session;
}

/**
 * 销毁会话
 * @param {string} sessionId - 会话ID
 */
function destroySession(sessionId) {
    if (activeSessions.has(sessionId)) {
        const session = activeSessions.get(sessionId);
        activeSessions.delete(sessionId);
        logWithTime(`[AUTH] Session destroyed for user: ${session.username}`);
    }
}

/**
 * 清理过期会话 (定期执行)
 */
function cleanupExpiredSessions() {
    const now = Date.now();
    let cleanedCount = 0;

    for (const [sessionId, session] of activeSessions.entries()) {
        if (now > session.expiresAt) {
            activeSessions.delete(sessionId);
            cleanedCount++;
        }
    }

    if (cleanedCount > 0) {
        logWithTime(`[AUTH] Cleaned up ${cleanedCount} expired sessions`);
    }
}

/**
 * 认证中间件 - 检查是否已登录
 */
function requireAuth(req, res, next) {
    // 如果认证被禁用，直接通过
    if (!config.AUTH_ENABLED) {
        return next();
    }

    const sessionId = req.cookies?.sessionId;
    const session = validateSession(sessionId);

    if (!session) {
        // 如果是API请求，返回401
        if (req.path.startsWith('/web/') || req.path.startsWith('/api/')) {
            return res.status(401).json({ error: 'Authentication required' });
        }

        // 否则重定向到登录页
        return res.redirect('/login');
    }

    // 将会话信息添加到请求对象
    req.session = session;
    next();
}

/**
 * 登录中间件 - 处理登录请求
 */
function handleLogin(req, res) {
    const { username, password } = req.body;

    if (!username || !password) {
        return res.status(400).json({ error: 'Username and password required' });
    }

    if (!validateCredentials(username, password)) {
        logWithTime(`[AUTH] Failed login attempt for user: ${username} from IP: ${req.ip}`);
        return res.status(401).json({ error: 'Invalid credentials' });
    }

    // 创建会话
    const sessionId = createSession(username);

    // 设置Cookie
    res.cookie('sessionId', sessionId, {
        httpOnly: true,
        secure: false, // 开发环境设为false，生产环境应为true
        sameSite: 'strict',
        maxAge: config.SESSION_EXPIRE_HOURS * 60 * 60 * 1000
    });

    logWithTime(`[AUTH] Successful login for user: ${username} from IP: ${req.ip}`);
    res.json({ success: true, message: 'Login successful' });
}

/**
 * 登出中间件 - 处理登出请求
 */
function handleLogout(req, res) {
    const sessionId = req.cookies?.sessionId;

    if (sessionId) {
        destroySession(sessionId);
    }

    res.clearCookie('sessionId');
    res.json({ success: true, message: 'Logout successful' });
}

/**
 * 获取当前会话信息
 */
function getSessionInfo(req, res) {
    if (!config.AUTH_ENABLED) {
        return res.json({ authenticated: false, authEnabled: false });
    }

    const sessionId = req.cookies?.sessionId;
    const session = validateSession(sessionId);

    if (!session) {
        return res.json({ authenticated: false, authEnabled: true });
    }

    res.json({
        authenticated: true,
        authEnabled: true,
        username: session.username,
        loginTime: session.createdAt,
        expiresAt: session.expiresAt
    });
}

// 定期清理过期会话 (每小时执行一次)
setInterval(cleanupExpiredSessions, 60 * 60 * 1000);

module.exports = {
    requireAuth,
    handleLogin,
    handleLogout,
    getSessionInfo,
    validateSession,
    createSession,
    destroySession
};
