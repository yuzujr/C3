# ScreenUploader 配置安全指南

## 🔐 安全配置方法

为了保护敏感信息（如密码、密钥），请使用以下任一方法配置：

### 方法一：环境变量配置 (推荐)

1. **复制环境变量模板**：
   ```bash
   cp .env.example .env
   ```

2. **编辑 .env 文件**：
   ```bash
   # 修改为你的安全密码
   AUTH_USERNAME=your_username
   AUTH_PASSWORD=your_secure_password_123
   SESSION_SECRET=your_very_long_random_session_secret_at_least_32_characters_long
   ```

3. **重启服务器**：
   ```bash
   node app.js
   ```

### 方法二：外部配置文件

1. **编辑 server.config.json**（首次运行会自动创建）：
   ```json
   {
       "auth": {
           "enabled": true,
           "username": "your_username",
           "password": "your_secure_password_123",
           "sessionSecret": "auto-generated-random-secret",
           "sessionExpireHours": 24
       },
       "server": {
           "httpPort": 4000,
           "wsPort": 8080,
           "host": "0.0.0.0"
       }
   }
   ```

### 方法三：系统环境变量

在系统级别设置环境变量：

**Windows (PowerShell)**：
```powershell
$env:AUTH_PASSWORD="your_secure_password_123"
$env:SESSION_SECRET="your_very_long_random_session_secret"
node app.js
```

**Linux/Mac**：
```bash
export AUTH_PASSWORD="your_secure_password_123"
export SESSION_SECRET="your_very_long_random_session_secret"
node app.js
```

## 🛡️ 配置优先级

系统会按以下顺序加载配置（高优先级覆盖低优先级）：

1. **环境变量** (最高优先级)
2. **外部配置文件** (server.config.json)
3. **默认值** (最低优先级)

## ⚠️ 安全建议

### 必须修改的配置
- `AUTH_PASSWORD`: 使用强密码（至少8位，包含字母数字特殊字符）
- `SESSION_SECRET`: 使用长随机字符串（至少32位）

### 密码强度要求
- ✅ 至少8个字符
- ✅ 包含大小写字母
- ✅ 包含数字
- ✅ 包含特殊字符
- ❌ 不要使用常见密码
- ❌ 不要使用个人信息

### 生产环境建议
- 使用HTTPS (考虑反向代理如Nginx)
- 限制访问IP (防火墙设置)
- 定期更换密码和密钥
- 启用访问日志监控
- 使用强随机SESSION_SECRET

## 🔍 配置验证

服务器启动时会自动检查配置安全性：

- ⚠️ 使用默认密码时会显示警告
- ⚠️ 密码过短时会显示警告  
- ⚠️ SESSION_SECRET过短时会显示警告
- ⚠️ 生产环境不安全配置会显示警告

## 📁 文件安全

以下文件包含敏感信息，已加入 `.gitignore`：
- `.env` - 环境变量文件
- `server.config.json` - 外部配置文件

这些文件不会被提交到版本控制系统。

## 🚀 快速安全部署

1. 克隆代码到服务器
2. 复制 `.env.example` 为 `.env`
3. 修改 `.env` 中的密码和密钥
4. 安装依赖：`npm install`
5. 启动服务：`node app.js`
6. 检查启动日志，确保无安全警告

## ❌ 禁用认证（仅开发环境）

如需临时禁用认证：
```bash
# 环境变量方式
AUTH_ENABLED=false node app.js

# 或在 .env 文件中设置
AUTH_ENABLED=false
```

**注意**：生产环境强烈建议保持认证开启！
