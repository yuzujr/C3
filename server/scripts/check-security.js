#!/usr/bin/env node
// 配置验证脚本
// 检查当前配置的安全性

const path = require('path');
require('dotenv').config({ path: path.join(__dirname, '..', '.env') });

function loadConfig() {
    return {
        auth: {
            enabled: process.env.AUTH_ENABLED !== 'false',
            username: process.env.AUTH_USERNAME || 'admin',
            password: process.env.AUTH_PASSWORD || 'CHANGE_ME_PLEASE',
            sessionSecret: process.env.SESSION_SECRET || 'default-secret',
            sessionExpireHours: parseInt(process.env.SESSION_EXPIRE_HOURS) || 24
        },
        server: {
            port: parseInt(process.env.PORT) || 3000,
            host: process.env.HOST || '0.0.0.0'
        }
    };
}

function validatePassword(password) {
    const issues = [];

    if (!password || password === 'CHANGE_ME_PLEASE' || password === '请修改这个密码!') {
        issues.push('使用默认密码');
    }

    if (password && password.length < 8) {
        issues.push('密码太短（建议至少8位）');
    }

    if (password && !/[A-Z]/.test(password)) {
        issues.push('缺少大写字母');
    }

    if (password && !/[a-z]/.test(password)) {
        issues.push('缺少小写字母');
    }

    if (password && !/[0-9]/.test(password)) {
        issues.push('缺少数字');
    }

    if (password && !/[^A-Za-z0-9]/.test(password)) {
        issues.push('缺少特殊字符');
    }

    return issues;
}

function validateSessionSecret(secret) {
    const issues = [];

    if (!secret) {
        issues.push('会话密钥为空');
    } else if (secret.length < 32) {
        issues.push('会话密钥太短（建议至少32位）');
    }

    return issues;
}

function main() {
    console.log('🔐 C3 配置安全检查');
    console.log('=====================================\n');

    const config = loadConfig();
    if (!config) {
        process.exit(1);
    }

    let score = 100;
    const issues = [];

    // 检查认证状态
    if (!config.auth.enabled) {
        console.log('⚠️  认证已禁用 - 任何人都可以访问控制台');
        issues.push('认证被禁用');
        score -= 50;
    } else {
        console.log('✅ 认证已启用');
    }

    // 检查用户名
    console.log(`👤 用户名: ${config.auth.username}`);
    if (config.auth.username === 'admin') {
        issues.push('使用默认用户名');
        score -= 10;
    }

    // 检查密码
    const passwordIssues = validatePassword(config.auth.password);
    if (passwordIssues.length === 0) {
        console.log('✅ 密码强度良好');
    } else {
        console.log('❌ 密码问题:');
        passwordIssues.forEach(issue => {
            console.log(`   - ${issue}`);
            issues.push(`密码${issue}`);
        });
        score -= passwordIssues.length * 10;
    }

    // 检查会话密钥
    const secretIssues = validateSessionSecret(config.auth.sessionSecret);
    if (secretIssues.length === 0) {
        console.log('✅ 会话密钥安全');
    } else {
        console.log('❌ 会话密钥问题:');
        secretIssues.forEach(issue => {
            console.log(`   - ${issue}`);
            issues.push(issue);
        });
        score -= secretIssues.length * 15;
    }

    // 检查配置来源
    console.log('\n📋 配置来源:');
    if (process.env.AUTH_PASSWORD) {
        console.log('✅ 密码来自环境变量');
    } else {
        console.log('⚠️  密码来自配置文件');
    }

    if (process.env.SESSION_SECRET) {
        console.log('✅ 会话密钥来自环境变量');
    } else {
        console.log('⚠️  会话密钥来自配置文件');
    }

    // 显示安全评分
    console.log('\n🏆 安全评分:');
    if (score >= 90) {
        console.log(`✅ ${score}/100 - 优秀`);
    } else if (score >= 70) {
        console.log(`⚠️  ${score}/100 - 良好`);
    } else if (score >= 50) {
        console.log(`⚠️  ${score}/100 - 一般`);
    } else {
        console.log(`❌ ${score}/100 - 需要改进`);
    }

    // 提供改进建议
    if (issues.length > 0) {
        console.log('\n💡 改进建议:');
        if (issues.some(i => i.includes('密码'))) {
            console.log('   - 运行 npm run setup 设置强密码');
        }
        if (issues.some(i => i.includes('用户名'))) {
            console.log('   - 修改默认用户名 admin');
        }
        if (issues.some(i => i.includes('会话密钥'))) {
            console.log('   - 使用更长的随机会话密钥');
        }
        if (issues.some(i => i.includes('认证'))) {
            console.log('   - 在生产环境中启用认证');
        }
    }

    console.log('\n=====================================');
    console.log('🔐 检查完成');
}

main();
