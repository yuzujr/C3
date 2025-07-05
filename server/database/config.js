// database/config.js
// 数据库配置文件

module.exports = {
    development: {
        username: process.env.DB_USER || 'postgres',
        password: process.env.DB_PASSWORD || 'password',
        database: process.env.DB_NAME || 'c3_db_dev',
        host: process.env.DB_HOST || 'localhost',
        port: process.env.DB_PORT || 5432,
        dialect: 'postgres',
        logging: false, // 彻底关闭SQL日志输出
        pool: {
            max: 5,
            min: 1,
            acquire: 30000,
            idle: 10000
        }
    },
    production: {
        username: process.env.DB_USER || 'postgres',
        password: process.env.DB_PASSWORD || 'password',
        database: process.env.DB_NAME || 'c3_db',
        host: process.env.DB_HOST || 'localhost',
        port: process.env.DB_PORT || 5432,
        dialect: 'postgres',
        logging: false, // 生产环境总是关闭SQL日志
        pool: {
            max: 10,
            min: 2,
            acquire: 30000,
            idle: 10000
        }
    }
};
