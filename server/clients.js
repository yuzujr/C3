// ---- 热加载 clients.json ----
let clientsCache = {};
const clientsJsonPath = path.join(__dirname, 'clients.json');

// 初始加载
function loadClientsFromDisk() {
  try {
    if (fs.existsSync(clientsJsonPath)) {
      const data = fs.readFileSync(clientsJsonPath, 'utf-8').trim();
      clientsCache = data ? JSON.parse(data) : {};
      logWithTime('[CLIENTS] Reloaded clients.json');
    } else {
      clientsCache = {};
    }
  } catch (err) {
    errorWithTime('[CLIENTS] Failed to read clients.json:', err.message);
    clientsCache = {};
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
