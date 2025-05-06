const express = require('express');
const multer  = require('multer');
const fs = require('fs');
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = 4000;

app.use(express.json());
app.use(cors())


const uploadDir = path.join(__dirname, 'uploads');
const logsDir = path.join(__dirname, 'logs');

// 创建 logs 目录
if (!fs.existsSync(logsDir)) {
  fs.mkdirSync(logsDir, { recursive: true });
  logWithTime('[INIT] Create \"logs\" directory:', logsDir);
}
// 创建 uploads 目录
if (!fs.existsSync(uploadDir)) {
  fs.mkdirSync(uploadDir, { recursive: true });
  logWithTime('[INIT] Create \"uploads\" directory:', uploadDir);
}

// 设置存储规则
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    const now = new Date();
    const year = now.getFullYear();
    const month = String(now.getMonth() + 1).padStart(2, '0');
    const day = String(now.getDate()).padStart(2, '0');
    const hour = String(now.getHours()).padStart(2, '0');

    const folderName = `${year}-${month}-${day}-${hour}`;
    const timeBasedDir = path.join(uploadDir, folderName);

    if (!fs.existsSync(timeBasedDir)) {
      fs.mkdirSync(timeBasedDir, { recursive: true });
      logWithTime('[INIT] Create time based subdirectory:', timeBasedDir);
    }

    cb(null, timeBasedDir);
  },
  filename: (req, file, cb) => {
    const safeName = file.originalname.replace(/[^a-zA-Z0-9.\-_]/g, '_');
    cb(null, safeName);
  }
});

const upload = multer({ storage });

app.post('/upload', upload.single('file'), (req, res) => {
  logWithTime('[UPLOAD] Incoming upload request received.');

  if (!req.file) {
    errorWithTime('[UPLOAD] ❌ No file received.');
    return res.status(400).send('[SERVER] No file received.\n');
  }

  logWithTime('[UPLOAD] ✅ File received successfully:');
  logWithTime(` - Original name: ${req.file.originalname}`);
  logWithTime(` - Saved as:      ${req.file.filename}`);
  logWithTime(` - MIME type:     ${req.file.mimetype}`);
  logWithTime(` - Path:          ${req.file.path}`);
  logWithTime(` - Size:          ${req.file.size} bytes\n`);

  return res.send('[SERVER] Upload success: ' + req.file.filename + '\n');
});

app.listen(PORT, '0.0.0.0', () => {
  logWithTime(`Server is running on http://0.0.0.0:${PORT}`);
});


// --- 日志辅助函数 ---
function getCurrentLogFilePath() {
  const now = new Date();
  const year = now.getFullYear();
  const month = String(now.getMonth() + 1).padStart(2, '0');
  const day = String(now.getDate()).padStart(2, '0');
  const hour = String(now.getHours()).padStart(2, '0');
  return path.join(logsDir, `${year}-${month}-${day}-${hour}.log`);
}

function getBeijingTime() {
  const now = new Date(Date.now() + 8 * 60 * 60 * 1000);
  return now.toISOString().replace('T', ' ').slice(0, 19);
}

function logWithTime(...args) {
  const timeStr = getBeijingTime();
  const message = `[${timeStr}] ` + args.join(' ');
  console.log(message);
  fs.appendFileSync(getCurrentLogFilePath(), message + '\n', 'utf8', (err) => {
  if (err) console.error('Failed to write log:', err);
});
}

function errorWithTime(...args) {
  const timeStr = getBeijingTime();
  const message = `[${timeStr}] ` + args.join(' ');
  console.error(message);
  fs.appendFileSync(getCurrentLogFilePath(), message + '\n', 'utf8', (err) => {
  if (err) console.error('Failed to write log:', err);
});
}
