const express = require('express');
const multer = require('multer');
const fs = require('fs');
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = 4000;

app.use(express.json());
app.use(cors());

const uploadRootDir = path.join(__dirname, 'uploads');
const clientsJsonPath = path.join(__dirname, 'clients.json');


// 创建 uploads 目录
if (!fs.existsSync(uploadRootDir)) {
  fs.mkdirSync(uploadRootDir, { recursive: true });
  logWithTime('[INIT] Create \"uploads\" directory:', uploadRootDir);
}


// 读取 clients.json（人类可读别名）
function loadClients() {
  if (fs.existsSync(clientsJsonPath)) {
    return JSON.parse(fs.readFileSync(clientsJsonPath, 'utf-8'));
  }
  return {};
}


// ---- Multer 设置 ----
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    const clientId = req.query.client_id;
    if (!clientId || !/^[a-f0-9-]{36}$/.test(clientId)) {
      return cb(new Error("Missing or invalid client_id"));
    }

    const clients = loadClients();
    const alias = clients[clientId];
    let folderName = clientId;

    if (alias) {
      const preferredName = `${alias}_${clientId}`;
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
    const year = now.getFullYear(), month = String(now.getMonth() + 1).padStart(2, '0'), day = String(now.getDate()).padStart(2, '0'), hour = String(now.getHours()).padStart(2, '0');
    const timeFolder = `${year}-${month}-${day}-${hour}`;
    const fullDir = path.join(uploadRootDir, folderName, timeFolder);

    fs.mkdirSync(fullDir, { recursive: true });
    logWithTime(`[UPLOAD] Save to ${fullDir}`);
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
  const clientId = req.query.client_id;
  logWithTime('[UPLOAD] Incoming upload from client:', clientId);

  if (!req.file) {
    errorWithTime('[UPLOAD] ❌ No file received.');
    return res.status(400).send('No file received.\n');
  }

  logWithTime('[UPLOAD] ✅ File uploaded:', req.file.path);
  return res.status(201).send('Upload success: ' + req.file.filename + '\n');
});

// ---- 启动服务器 ----
app.listen(PORT, '0.0.0.0', () => {
  logWithTime(`Server running at http://0.0.0.0:${PORT}`);
});
