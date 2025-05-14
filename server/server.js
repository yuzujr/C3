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

// ---- 日志工具 ----
const { logWithTime, errorWithTime } = require('./logger');
// ---- 设置上传目录 ----
const uploadRootDir = path.join(__dirname, 'uploads');
// ---- 热加载 clients.json ----
const { getClients } = require('./clients');

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

    if (clientId && /^[a-f0-9-]+$/i.test(clientId)) {
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
    let folderName = clientId;

    if (alias) {
      const preferredName = `${alias}`;
      const oldPath = path.join(uploadRootDir, clientId);
      const newPath = path.join(uploadRootDir, preferredName);

      if (fs.existsSync(oldPath) && !fs.existsSync(newPath)) {
        fs.renameSync(oldPath, newPath);
        logWithTime(`[RENAME] ${oldPath} → ${newPath}`);
      }

      folderName = preferredName;
    }

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

// ---- 上传处理 ----
app.post('/upload', upload.single('file'), (req, res) => {
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
  return res.status(201).send('Upload success: ' + req.file.filename);
});

// ---- 启动服务器 ----
app.listen(PORT, '0.0.0.0', () => {
  logWithTime(`[INIT] Server running at http://0.0.0.0:${PORT}`);
});
