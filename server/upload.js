// 文件上传处理模块
// 处理Multer配置和文件上传逻辑

const multer = require('multer');
const fs = require('fs');
const path = require('path');
const config = require('./config');
const { logWithTime, errorWithTime } = require('./logger');
const clientManager = require('./client-manager');

/**
 * 创建上传目录（如果不存在）
 */
function ensureUploadDirectory() {
    if (!fs.existsSync(config.UPLOADS_DIR)) {
        fs.mkdirSync(config.UPLOADS_DIR, { recursive: true });
        logWithTime('[INIT] Created "uploads" directory:', config.UPLOADS_DIR);
    }
}

/**
 * Multer存储配置
 */
const storage = multer.diskStorage({
    destination: async (req, file, cb) => {
        try {
            const clientId = req.clientId;
            if (!clientId) {
                return cb(new Error("Missing or invalid client_id"));
            }
            
            const alias = await clientManager.getAlias(clientId);
            let folderName = clientId; // Use client_id for directory structure, not alias

            // 时间子目录
            const now = new Date();
            const year = now.getFullYear();
            const month = String(now.getMonth() + 1).padStart(2, '0');
            const day = String(now.getDate()).padStart(2, '0');
            const hour = String(now.getHours()).padStart(2, '0');
            const timeFolder = `${year}-${month}-${day}-${hour}`;

            const fullDir = path.join(config.UPLOADS_DIR, folderName, timeFolder);
            fs.mkdirSync(fullDir, { recursive: true });

            cb(null, fullDir);
        } catch (error) {
            cb(error);
        }
    },

    filename: (req, file, cb) => {
        // 生成安全的文件名，移除特殊字符
        const safeName = file.originalname.replace(/[^a-zA-Z0-9.\-_]/g, '_');
        cb(null, safeName);
    }
});

/**
 * 文件过滤器
 * @param {object} req - 请求对象
 * @param {object} file - 文件对象
 * @param {function} cb - 回调函数
 */
function fileFilter(req, file, cb) {
    // 检查文件扩展名
    const ext = path.extname(file.originalname).toLowerCase();

    if (config.ALLOWED_FILE_TYPES.includes(ext)) {
        cb(null, true);
    } else {
        cb(new Error(`File type ${ext} not allowed`), false);
    }
}

/**
 * 创建Multer实例
 */
const upload = multer({
    storage,
    fileFilter,
    limits: {
        fileSize: config.MAX_FILE_SIZE
    }
});

/**
 * 处理文件上传错误
 * @param {Error} error - 上传错误
 * @param {object} req - 请求对象
 * @param {object} res - 响应对象
 * @param {function} next - next函数
 */
function handleUploadError(error, req, res, next) {
    if (error instanceof multer.MulterError) {
        if (error.code === 'LIMIT_FILE_SIZE') {
            return res.status(400).send('File too large');
        }
        if (error.code === 'LIMIT_UNEXPECTED_FILE') {
            return res.status(400).send('Unexpected file field');
        }
    }

    if (error.message) {
        return res.status(400).send(error.message);
    }

    next(error);
}

/**
 * 初始化上传模块
 */
function initUploadModule() {
    ensureUploadDirectory();
}

module.exports = {
    upload,
    handleUploadError,
    initUploadModule,
    ensureUploadDirectory
};
