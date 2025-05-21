// ---- 引入所需模块 ----
const express = require('express');
const multer = require('multer');
const fs = require('fs');
const path = require('path');
const cors = require('cors');

// ---- 初始化 ----
const app = express();
const PORT = 4000;
app.use(express.json());
app.use(cors());
app.use(express.static(path.join(__dirname, 'public')));
app.use('/uploads', express.static(path.join(__dirname, 'uploads')));

// ---- 日志工具 ----
const { logWithTime, errorWithTime, getBeijingTime } = require('./logger');
// ---- 设置上传目录 ----
const uploadRootDir = path.join(__dirname, 'uploads');
// ---- 热加载 clients.json ----
const { getClients } = require('./clients');
// ---- 获取、添加指令 ----
const {getAndClearCommands, addCommand, clearClientCommands} = require('./commands');
const { clear } = require('console');

// ---- 初始化 uploads 目录 ----
if (!fs.existsSync(uploadRootDir)) {
  fs.mkdirSync(uploadRootDir, { recursive: true });
  logWithTime('[INIT] Created "uploads" directory:', uploadRootDir);
}

// ---- 中间件：提前解析 client_id ----
app.use((req, res, next) => {
  try {
    const url = new URL(req.url, `http://${req.headers.host}`);
    const clientId = url.searchParams.get('client_id');

    if (clientId) {
      req.clientId = clientId;
    } else {
      req.clientId = null;
    }

    next();
  } catch (e) {
    req.clientId = null;
    next(); // 保证不中断流程
  }
});

// ---- Multer 设置 ----
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    const clientId = req.clientId;
    if (!clientId) {
      return cb(new Error("Missing or invalid client_id"));
    }

    const clients = getClients();
    const alias = clients[clientId];
    let folderName = alias ? alias : clientId;

    // 时间子目录
    const now = new Date();
    const year = now.getFullYear();
    const month = String(now.getMonth() + 1).padStart(2, '0');
    const day = String(now.getDate()).padStart(2, '0');
    const hour = String(now.getHours()).padStart(2, '0');
    const timeFolder = `${year}-${month}-${day}-${hour}`;

    const fullDir = path.join(uploadRootDir, folderName, timeFolder);
    fs.mkdirSync(fullDir, { recursive: true });

    cb(null, fullDir);
  },

  filename: (req, file, cb) => {
    const safeName = file.originalname.replace(/[^a-zA-Z0-9.\-_]/g, '_');
    cb(null, safeName);
  }
});

const upload = multer({ storage });

// ==== 客户端接口 ====

// ---- 客户端上传截图 ----
app.post('/client/upload_screenshot', upload.single('file'), (req, res) => {
  const clientId = req.clientId;
  const clients = getClients();
  const alias = clients[clientId];
  const displayName = alias ? `${alias} (${clientId})` : clientId;
  
  logWithTime('[UPLOAD] Incoming upload from:', displayName);

  if (!req.file) {
    errorWithTime('[UPLOAD] ❌ No file received.');
    return res.status(400).send('No file received.');
  }

  logWithTime('[UPLOAD] ✅ File uploaded successfully:');
  logWithTime(` - Path: ${req.file.path}`);
  logWithTime(` - Size: ${req.file.size} bytes\n`);
  return res.status(201).send('Screenshot saved successfully: ' + req.file.filename);
});

// --- 客户端获取命令 ---
app.get('/client/commands', (req, res) => {
    const clientId = req.query.client_id;
    const clients = getClients();
    const alias = clients[clientId];
    const displayName = alias ? `${alias} (${clientId})` : clientId;
    
    if (!clientId) {
        return res.status(400).send('Missing client_id');
    }

    const commands = getAndClearCommands(clientId);
    if (commands.length > 0) {
        logWithTime(`[COMMAND] ${displayName} -> ${JSON.stringify(commands)}`);
    }
    return res.json({commands});
});

// ---- 客户端上传配置 ----
app.post('/client/upload_client_config', (req, res) => {
  const clientId = req.query.client_id;
  const config = req.body;

  if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
    errorWithTime('[CONFIG] ❌ Missing or invalid client_id');
    return res.status(400).send('Missing or invalid client_id');
  }

  const clients = getClients();
  const alias = clients[clientId] || clientId;  // 使用别名作为文件夹名，若无则用原ID
  const clientDir = path.join(uploadRootDir, alias);

  try {
    fs.mkdirSync(clientDir, { recursive: true });
    const configPath = path.join(clientDir, 'config.json');

    const fullConfig = {
      ...config,
      lastUpload: getBeijingTime(),
    };

    fs.writeFileSync(configPath, JSON.stringify(fullConfig, null, 2));
    logWithTime(`[CONFIG] ✅ Saved config to ${configPath}`);
    return res.status(200).send('Config saved successfully');
  } catch (e) {
    errorWithTime('[CONFIG] ❌ Failed to save config:', e);
    return res.status(500).send('Failed to save config');
  }
});

// ==== 网页接口 ====

app.get('/web/clients', (req, res) => {
  if (!fs.existsSync(uploadRootDir)) return res.json([]);
  const clients = fs.readdirSync(uploadRootDir);
  res.json(clients);
});

app.get('/web/screenshots/:client_id', (req, res) => {
  const clientId = req.params.client_id;
  const sinceTimestamp = req.query.since ? parseInt(req.query.since) : 0;
  
  const clientPath = path.join(uploadRootDir, clientId);
  if (!fs.existsSync(clientPath)) return res.json([]);

  const result = [];
  // 获取所有日期目录
  const dateDirs = fs.readdirSync(clientPath)
    .filter(dir => {
      const fullPath = path.join(clientPath, dir);
      return fs.statSync(fullPath).isDirectory();
    })
    .sort((a, b) => b.localeCompare(a)); // 时间目录从新到旧排序
  
  // 遍历排序后的日期目录
  dateDirs.forEach(dateDir => {
    const fullDatePath = path.join(clientPath, dateDir);
    const files = fs.readdirSync(fullDatePath)
      .sort((a, b) => {
        // 使用文件修改时间排序
        const statA = fs.statSync(path.join(fullDatePath, a));
        const statB = fs.statSync(path.join(fullDatePath, b));
        return statB.mtime.getTime() - statA.mtime.getTime(); // 从新到旧
      });
      
    // 遍历并过滤图片
    files.forEach(file => {
      const filePath = path.join(fullDatePath, file);
      const fileStat = fs.statSync(filePath);
      
      // 只返回指定时间之后的图片
      if (fileStat.mtimeMs > sinceTimestamp) {
        result.push(`/uploads/${clientId}/${dateDir}/${file}`);
      }
    });
  });

  res.json(result);
});

app.get('/web/config/:clientId', (req, res) => {
  const clientId = req.params.clientId;

  if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
    return res.status(400).send('Missing or invalid clientId');
  }

  const configPath = path.join(uploadRootDir, clientId, 'config.json');

  if (!fs.existsSync(configPath)) {
    return res.status(404).send('Client config not found');
  }

  try {
    const raw = fs.readFileSync(configPath, 'utf-8').trim();
    const config = raw ? JSON.parse(raw) : {};
    res.json(config.api);
  } catch (e) {
    errorWithTime('[CONFIG] ❌ Failed to read config:', e);
    res.status(500).send('Failed to read config');
  }
});

app.post('/web/send_commands', (req, res) => {
  const { client_id, command } = req.body;
  if (!client_id || !command) {
    return res.status(400).send('Missing client_id or command');
  }

  try {
    addCommand(client_id, command);
    res.sendStatus(200);
  } catch (e) {
    errorWithTime('[COMMAND] ❌ Failed to add command:', e);
    res.status(500).send('Failed to add command');
  }
});

app.post('/web/clear_commands', (req, res) => {
  const { client_id } = req.body;
  if (!client_id) {
    return res.status(400).send('Missing client_id');
  }

  try {
    clearClientCommands(client_id);
    logWithTime(`[COMMAND] Cleared commands for ${client_id}`);
    res.sendStatus(200);
  } catch (e) {
    errorWithTime('[COMMAND] ❌ Failed to clear commands:', e);
    res.status(500).send('Failed to clear commands');
  }
});

// ---- 启动服务器 ----
app.listen(PORT, '0.0.0.0', () => {
  logWithTime(`[INIT] Server running at http://127.0.0.1:${PORT}`);
});