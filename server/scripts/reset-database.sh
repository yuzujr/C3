#!/bin/bash

# 清空数据库脚本
# 用于重置数据库到初始状态

echo "=== C3 数据库清空脚本 ==="
echo ""

# 检查是否在正确的目录
if [ ! -f "package.json" ]; then
    echo "错误: 请在 server 目录中运行此脚本"
    exit 1
fi

# 检查环境变量文件
if [ ! -f ".env" ]; then
    echo "错误: 未找到 .env 文件，请先配置环境变量"
    echo "运行: cp .env.example .env"
    exit 1
fi

echo "⚠️  警告: 此操作将清空所有数据库数据！"
echo "包括: 用户、客户端、命令日志、截图记录等"
echo ""
read -p "确定要继续吗？(y/N): " confirm

if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
    echo "操作已取消"
    exit 0
fi

echo ""
echo "正在清空数据库..."

# 方法1: 使用 psql 命令直接清空
echo "1. 尝试使用 psql 命令清空数据库..."
source .env
if command -v psql &> /dev/null; then
    echo "正在连接数据库 $DB_HOST:$DB_PORT/$DB_NAME..."
    PGPASSWORD="$DB_PASSWORD" psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "
        DROP TABLE IF EXISTS audit_logs CASCADE;
        DROP TABLE IF EXISTS user_sessions CASCADE;
        DROP TABLE IF EXISTS screenshots CASCADE;
        DROP TABLE IF EXISTS command_logs CASCADE;
        DROP TABLE IF EXISTS clients CASCADE;
        DROP TABLE IF EXISTS users CASCADE;
        DROP TABLE IF EXISTS \"SequelizeMeta\" CASCADE;
    "
    if [ $? -eq 0 ]; then
        echo "✓ 数据库表已清空"
    else
        echo "✗ 使用 psql 清空失败，尝试方法2..."
    fi
else
    echo "未找到 psql 命令，尝试方法2..."
fi

# 方法2: 使用 Node.js 脚本清空
echo ""
echo "2. 使用 Node.js 脚本重新初始化数据库..."
node -e "
const { sequelize } = require('./database/models');
async function resetDB() {
    try {
        await sequelize.authenticate();
        console.log('连接数据库成功');
        
        // 强制同步，删除所有表并重新创建
        await sequelize.sync({ force: true });
        console.log('✓ 数据库已重置');
        
        await sequelize.close();
        console.log('数据库连接已关闭');
    } catch (error) {
        console.error('重置数据库失败:', error);
        process.exit(1);
    }
}
resetDB();
"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ 数据库清空完成！"
    echo ""
    echo "下一步操作:"
    echo "1. 运行: node database/init.js"
    echo "2. 或者按照 SETUP.md 教程重新配置"
else
    echo "✗ 清空数据库失败"
    exit 1
fi
