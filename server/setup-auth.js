#!/usr/bin/env node
// 快速安全配置脚本
// 用于设置安全的用户名和密码

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const readline = require('readline');

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

function generateRandomSecret() {
    return crypto.randomBytes(32).toString('hex');
}

async function question(query) {
    return new Promise(resolve => rl.question(query, resolve));
}

async function main() {
    console.log('🔐 ScreenUploader 安全配置向导');
    console.log('=====================================\n');

    try {
        // 获取用户输入
        const username = await question('请输入用户名 (默认: admin): ') || 'admin';
        const password = await question('请输入密码 (至少8位): ');

        if (!password || password.length < 8) {
            console.log('❌ 密码不能为空且至少需要8位字符！');
            process.exit(1);
        }

        const confirmPassword = await question('请确认密码: ');
        if (password !== confirmPassword) {
            console.log('❌ 两次输入的密码不匹配！');
            process.exit(1);
        }

        // 生成配置
        const config = {
            auth: {
                enabled: true,
                username: username,
                password: password,
                sessionSecret: generateRandomSecret(),
                sessionExpireHours: 24
            },
            server: {
                port: 3000,
                host: "0.0.0.0"
            }
        };

        // 保存配置文件
        const configPath = path.join(__dirname, 'server.config.json');
        fs.writeFileSync(configPath, JSON.stringify(config, null, 4));

        console.log('\n✅ 配置已保存到 server.config.json');
        console.log('✅ 会话密钥已自动生成');
        console.log('\n🚀 现在可以启动服务器了：');
        console.log('   node app.js');
        console.log('\n⚠️  重要提醒：');
        console.log('   - server.config.json 包含敏感信息，请勿共享');
        console.log('   - 此文件已被 .gitignore 忽略，不会提交到版本控制');

    } catch (error) {
        console.error('❌ 配置过程出错：', error.message);
        process.exit(1);
    } finally {
        rl.close();
    }
}

main();
