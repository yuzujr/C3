#!/usr/bin/env node
// 测试数据库连接脚本

const path = require('path');
require('dotenv').config({ path: path.join(__dirname, '..', '.env') });
const { sequelize } = require('../database/models');

async function testConnection() {
    try {
        console.log('正在测试数据库连接...');
        
        // 测试连接
        await sequelize.authenticate();
        console.log('✓ 数据库连接成功');
        
        // 测试查询
        const result = await sequelize.query('SELECT version()');
        console.log('✓ PostgreSQL 版本:', result[0][0].version);
        
        // 关闭连接
        await sequelize.close();
        console.log('✓ 数据库连接已关闭');
        
    } catch (error) {
        console.error('✗ 数据库连接失败:', error.message);
        console.error('请检查：');
        console.error('1. PostgreSQL 是否已安装并运行');
        console.error('2. 数据库配置是否正确');
        console.error('3. 数据库用户是否有权限');
        process.exit(1);
    }
}

if (require.main === module) {
    testConnection();
}

module.exports = { testConnection };
