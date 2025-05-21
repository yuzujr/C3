const fs = require('fs');
const path = require('path');

// ---- 日志工具 ----
const { logWithTime, errorWithTime } = require('./logger');

// ---- 热加载 clients.json ----
let clientsCache = {};
const clientsJsonPath = path.join(__dirname, 'clients.json');
const uploadsDir = path.join(__dirname, 'uploads');

// 初始加载
function loadClientsFromDisk() {
  try {
    // 保存旧的客户端映射，用于检测变化
    const oldClientsMapping = JSON.parse(JSON.stringify(clientsCache));
    
    if (fs.existsSync(clientsJsonPath)) {
      const data = fs.readFileSync(clientsJsonPath, 'utf-8').trim();
      clientsCache = data ? JSON.parse(data) : {};
      logWithTime('[CLIENTS] Reloaded clients.json');
      
      // 检查是否有变化，并执行重命名操作
      handleClientNameChanges(oldClientsMapping, clientsCache);
    } else {
      clientsCache = {};
    }
  } catch (err) {
    errorWithTime('[CLIENTS] Failed to read clients.json:', err.message);
    clientsCache = {};
  }
}

// 处理客户端名称变化
function handleClientNameChanges(oldMapping, newMapping) {
  // 确保uploads目录存在
  if (!fs.existsSync(uploadsDir)) {
    return;
  }
  
  // 1. 首先处理ID到别名的映射变化
  for (const clientId in newMapping) {
    const newAlias = newMapping[clientId];
    const oldAlias = oldMapping[clientId];
    
    // 如果是新客户端或别名发生变化
    if (oldAlias === undefined || newAlias !== oldAlias) {
      // 检查是否存在以clientId命名的目录
      const clientIdDir = path.join(uploadsDir, clientId);
      if (fs.existsSync(clientIdDir) && fs.statSync(clientIdDir).isDirectory()) {
        // 目标目录路径
        const targetDir = path.join(uploadsDir, newAlias);
        
        // 检查目标目录是否已存在
        if (fs.existsSync(targetDir)) {
          logWithTime(`[CLIENTS] Cannot rename ${clientId} to ${newAlias}: Target directory already exists`);
          continue;
        }
        
        try {
          // 执行重命名
          fs.renameSync(clientIdDir, targetDir);
          logWithTime(`[CLIENTS] Renamed directory: ${clientId} -> ${newAlias}`);
        } catch (err) {
          errorWithTime(`[CLIENTS] Failed to rename ${clientId} to ${newAlias}:`, err.message);
        }
      }
    }
  }
  
  // 2. 然后处理别名到别名的映射变化
  // 检查每个客户端的别名变化
  for (const clientId in newMapping) {
    const newAlias = newMapping[clientId];
    const oldAlias = oldMapping[clientId];
    
    // 如果别名发生变化且旧别名不为空
    if (oldAlias && newAlias !== oldAlias) {
      // 检查是否存在以旧别名命名的目录
      const oldAliasDir = path.join(uploadsDir, oldAlias);
      if (fs.existsSync(oldAliasDir) && fs.statSync(oldAliasDir).isDirectory()) {
        // 目标目录路径
        const targetDir = path.join(uploadsDir, newAlias);
        
        // 检查目标目录是否已存在
        if (fs.existsSync(targetDir)) {
          logWithTime(`[CLIENTS] Cannot rename ${oldAlias} to ${newAlias}: Target directory already exists`);
          continue;
        }
        
        try {
          // 执行重命名
          fs.renameSync(oldAliasDir, targetDir);
          logWithTime(`[CLIENTS] Renamed directory: ${oldAlias} -> ${newAlias}`);
        } catch (err) {
          errorWithTime(`[CLIENTS] Failed to rename ${oldAlias} to ${newAlias}:`, err.message);
        }
      }
    }
  }
}

// 热加载函数（提供读取缓存）
function getClients() {
  return clientsCache;
}

// 启动时加载一次
loadClientsFromDisk();

// 文件监听（热更新）
fs.watchFile(clientsJsonPath, { interval: 1000 }, (curr, prev) => {
  if (curr.mtime !== prev.mtime) {
    logWithTime('[CLIENTS] Detected clients.json change, reloading...');
    loadClientsFromDisk();
  }
});

module.exports = { getClients };