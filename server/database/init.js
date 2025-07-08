// database/init.js
// 数据库初始化脚本

const bcrypt = require('bcryptjs');
const { sequelize, User, Client } = require('./models');
const { logWithTime, errorWithTime } = require('../logger');

/**
 * 初始化数据库
 */
async function initDatabase() {
  try {
    // 测试数据库连接
    await sequelize.authenticate();
    logWithTime('[DATABASE] Connection established successfully');

    // 同步数据库结构
    await sequelize.sync({ force: false });
    logWithTime('[DATABASE] Database synchronized');

    // 创建默认管理员用户
    await createDefaultUser();

    logWithTime('[DATABASE] Database initialization completed');
    return true;
  } catch (error) {
    errorWithTime('[DATABASE] Failed to initialize database:', error);
    return false;
  }
}

/**
 * 创建默认管理员用户
 */
async function createDefaultUser() {
  try {
    // 检查是否已有用户
    const userCount = await User.count();
    if (userCount === 0) {
      // 从配置文件读取默认用户信息
      const config = require('../config');
      const defaultUsername = config.AUTH_USERNAME || 'admin';
      const defaultPassword = config.AUTH_PASSWORD || 'admin123';

      // 加密密码
      const saltRounds = 10;
      const password_hash = await bcrypt.hash(defaultPassword, saltRounds);

      // 创建默认用户
      await User.create({
        username: defaultUsername,
        password_hash,
        role: 'admin'
      });

      logWithTime(`[DATABASE] Created default admin user: ${defaultUsername}`);
    } else {
      logWithTime('[DATABASE] Users already exist, skipping default user creation');
    }
  } catch (error) {
    errorWithTime('[DATABASE] Failed to create default user:', error);
  }
}

/**
 * 迁移现有数据
 */
async function migrateExistingData() {
  try {
    // 迁移 clients.json 数据
    await migrateClientsData();

    logWithTime('[DATABASE] Data migration completed');
  } catch (error) {
    errorWithTime('[DATABASE] Failed to migrate data:', error);
  }
}

/**
 * 迁移客户端数据
 */
async function migrateClientsData() {
  const fs = require('fs');
  const path = require('path');

  const clientsJsonPath = path.join(__dirname, '../clients.json');

  if (fs.existsSync(clientsJsonPath)) {
    try {
      const data = fs.readFileSync(clientsJsonPath, 'utf-8').trim();
      const clientsMapping = data ? JSON.parse(data) : {};

      for (const [client_id, alias] of Object.entries(clientsMapping)) {
        // 检查是否已存在
        const existingClient = await Client.findOne({ where: { client_id } });
        if (!existingClient) {
          await Client.create({
            client_id,
            alias,
            online_status: false
          });
          logWithTime(`[DATABASE] Migrated client: ${alias} (${client_id})`);
        }
      }

      // 备份原文件
      const backupPath = clientsJsonPath + '.backup';
      fs.copyFileSync(clientsJsonPath, backupPath);
      logWithTime(`[DATABASE] Backed up clients.json to ${backupPath}`);

    } catch (error) {
      errorWithTime('[DATABASE] Failed to migrate clients data:', error);
    }
  }
}

/**
 * 关闭数据库连接
 */
async function closeDatabase() {
  try {
    await sequelize.close();
    logWithTime('[DATABASE] Database connection closed');
  } catch (error) {
    errorWithTime('[DATABASE] Failed to close database:', error);
  }
}

module.exports = {
  initDatabase,
  migrateExistingData,
  closeDatabase
};

// 如果直接运行此脚本，则执行初始化
if (require.main === module) {
  initDatabase()
    .then(() => {
      console.log('数据库初始化完成');
      process.exit(0);
    })
    .catch(err => {
      console.error('数据库初始化失败:', err);
      process.exit(1);
    });
}
