// 认证模块
// 处理用户登录验证和会话管理 (使用数据库)

const authService = require('./services/auth-service');
const { logWithTime, errorWithTime } = require('./logger');

/**
 * 验证用户名和密码
 * @param {string} username - 用户名
 * @param {string} password - 密码
 * @returns {Promise<object>} 登录结果
 */
async function validateCredentials(username, password) {
  try {
    return await authService.login(username, password);
  } catch (error) {
    errorWithTime('[AUTH] Login validation failed:', error);
    return { success: false, message: '登录失败' };
  }
}

/**
 * 创建新会话
 * @param {string} username - 用户名
 * @param {string} password - 密码
 * @returns {Promise<string|null>} 会话ID
 */
async function createSession(username, password) {
  const loginResult = await validateCredentials(username, password);
  if (loginResult.success) {
    return loginResult.sessionId;
  }
  return null;
}

/**
 * 验证会话
 * @param {string} sessionId - 会话ID
 * @returns {Promise<object|null>} 会话信息或null
 */
async function validateSession(sessionId) {
  try {
    return await authService.validateSession(sessionId);
  } catch (error) {
    errorWithTime('[AUTH] Session validation failed:', error);
    return null;
  }
}

/**
 * 认证中间件
 * @param {object} req - 请求对象
 * @param {object} res - 响应对象
 * @param {function} next - 下一个中间件
 */
async function requireAuth(req, res, next) {
  try {
    const sessionId = req.cookies.sessionId;

    if (!sessionId) {
      return res.redirect('/login');
    }

    const session = await validateSession(sessionId);
    if (!session) {
      // 清除无效的cookie
      res.clearCookie('sessionId');
      return res.redirect('/login');
    }

    // 将用户信息添加到请求对象
    req.user = session.user;
    req.sessionId = session.sessionId;

    next();
  } catch (error) {
    errorWithTime('[AUTH] Authentication middleware error:', error);
    res.clearCookie('sessionId');
    res.redirect('/login');
  }
}

/**
 * 管理员权限中间件
 * @param {object} req - 请求对象
 * @param {object} res - 响应对象
 * @param {function} next - 下一个中间件
 */
async function requireAdmin(req, res, next) {
  try {
    await requireAuth(req, res, () => {
      if (req.user && req.user.role === 'admin') {
        next();
      } else {
        res.status(403).json({ success: false, message: '需要管理员权限' });
      }
    });
  } catch (error) {
    errorWithTime('[AUTH] Admin middleware error:', error);
    res.status(500).json({ success: false, message: '认证失败' });
  }
}

/**
 * 获取活跃会话统计
 * @returns {Promise<object>} 统计信息
 */
async function getSessionStats() {
  try {
    const activeCount = await authService.getActiveSessionCount();
    return {
      activeSessionCount: activeCount
    };
  } catch (error) {
    errorWithTime('[AUTH] Failed to get session stats:', error);
    return {
      activeSessionCount: 0
    };
  }
}

/**
 * 登录处理函数
 * @param {object} req - 请求对象
 * @param {object} res - 响应对象
 */
async function handleLogin(req, res) {
  const { username, password } = req.body;

  if (!username || !password) {
    return res.status(400).json({ error: 'Username and password required' });
  }

  try {
    const loginResult = await validateCredentials(username, password);

    if (!loginResult.success) {
      logWithTime(`[WEB] user: ${username} ip: ${req.ip} login failed - ${loginResult.message}`);
      return res.status(401).json({ error: loginResult.message });
    }

    // 设置Cookie
    res.cookie('sessionId', loginResult.sessionId, {
      httpOnly: true,
      secure: false, // 开发环境设为false，生产环境应为true
      sameSite: 'strict',
      maxAge: 24 * 60 * 60 * 1000 // 24小时
    });

    logWithTime(`[WEB] user: ${username} ip: ${req.ip} login successfully`);
    res.json({ success: true, message: 'Login successful' });
  } catch (error) {
    errorWithTime('[AUTH] Login handler error:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
}

/**
 * 登出处理函数
 * @param {object} req - 请求对象
 * @param {object} res - 响应对象
 */
async function handleLogout(req, res) {
  const sessionId = req.cookies?.sessionId;

  if (sessionId) {
    try {
      await authService.logout(sessionId);
    } catch (error) {
      errorWithTime('[AUTH] Session destruction failed:', error);
    }
  }

  res.clearCookie('sessionId');

  logWithTime(`[WEB] user logout successfully`);
  res.json({ success: true, message: 'Logout successful' });
}

/**
 * 获取当前会话信息
 * @param {object} req - 请求对象
 * @param {object} res - 响应对象
 */
async function getSessionInfo(req, res) {
  const config = require('./config');

  if (!config.AUTH_ENABLED) {
    return res.json({ authenticated: false, authEnabled: false });
  }

  const sessionId = req.cookies?.sessionId;
  const session = await validateSession(sessionId);

  if (!session) {
    return res.json({ authenticated: false, authEnabled: true });
  }

  res.json({
    authenticated: true,
    authEnabled: true,
    username: session.user.username,
    role: session.user.role,
    userId: session.user.id
  });
}

module.exports = {
  validateCredentials,
  createSession,
  validateSession,
  requireAuth,
  requireAdmin,
  getSessionStats,
  handleLogin,
  handleLogout,
  getSessionInfo
};
