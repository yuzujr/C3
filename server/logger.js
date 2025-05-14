// logger.js
const fs = require('fs');
const path = require('path');

const logsDir = path.join(__dirname, 'logs');

// 创建 logs 目录（如果不存在）
if (!fs.existsSync(logsDir)) {
  fs.mkdirSync(logsDir, { recursive: true });
  logWithTime('[INIT] Created "logs" directory:', logsDir);
}

// 获取当前日志文件路径（按小时记录）
function getCurrentLogFilePath() {
  const now = new Date();
  const y = now.getFullYear();
  const m = String(now.getMonth() + 1).padStart(2, '0');
  const d = String(now.getDate()).padStart(2, '0');
  const h = String(now.getHours()).padStart(2, '0');
  return path.join(logsDir, `${y}-${m}-${d}-${h}.log`);
}


// 获取北京时间字符串
function getBeijingTime() {
  return new Date(Date.now() + 8 * 60 * 60 * 1000).toISOString().replace('T', ' ').slice(0, 19);
}


// 日志输出函数
function logWithTime(...args) {
  const message = `[${getBeijingTime()}] ` + args.join(' ');
  console.log(message);
  fs.appendFileSync(getCurrentLogFilePath(), message + '\n');
}


// 错误日志输出
function errorWithTime(...args) {
  const message = `[${getBeijingTime()}] ` + args.join(' ');
  console.error(message);
  fs.appendFileSync(getCurrentLogFilePath(), message + '\n');
}


module.exports = {
  logWithTime,
  errorWithTime,
};
