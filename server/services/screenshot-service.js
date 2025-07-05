// 截图服务模块
// 处理截图相关的业务逻辑

const fs = require('fs');
const path = require('path');
const config = require('../config');
const { logWithTime, errorWithTime } = require('../logger');

/**
 * 获取客户端截图列表
 * @param {string} clientId - 客户端ID
 * @param {number} sinceTimestamp - 时间戳过滤器
 * @returns {Array<string>} 截图URL列表
 */
function getClientScreenshots(clientId, sinceTimestamp = 0) {
    const clientPath = path.join(config.UPLOADS_DIR, clientId);

    if (!fs.existsSync(clientPath)) {
        return [];
    }

    const result = [];

    try {
        // 获取所有日期目录
        const dateDirs = fs.readdirSync(clientPath).filter(item => {
            const fullPath = path.join(clientPath, item);
            return fs.statSync(fullPath).isDirectory();
        });

        dateDirs.forEach(dateDir => {
            const dateDirPath = path.join(clientPath, dateDir);
            const files = fs.readdirSync(dateDirPath);

            files.forEach(file => {
                const filePath = path.join(dateDirPath, file);
                const fileStat = fs.statSync(filePath);

                // 只返回指定时间之后的图片
                if (fileStat.mtimeMs > sinceTimestamp) {
                    result.push(`/uploads/${clientId}/${dateDir}/${file}`);
                }
            });
        });

        // 按时间排序（最新的在前）
        result.sort((a, b) => {
            const statA = fs.statSync(path.join(config.UPLOADS_DIR, a.replace('/uploads/', '')));
            const statB = fs.statSync(path.join(config.UPLOADS_DIR, b.replace('/uploads/', '')));
            return statB.mtimeMs - statA.mtimeMs;
        });

    } catch (error) {
        errorWithTime(`[SCREENSHOT] Error getting screenshots for ${clientId}:`, error);
    }

    return result;
}

/**
 * 删除指定时间范围内的截图
 * @param {string} clientId - 客户端ID
 * @param {number} hours - 删除多少小时前的截图
 * @returns {object} 删除结果
 */
function deleteScreenshotsByTime(clientId, hours) {
    const clientPath = path.join(config.UPLOADS_DIR, clientId);

    if (!fs.existsSync(clientPath)) {
        throw new Error('Client not found');
    }

    const cutoffTime = Date.now() - (hours * 60 * 60 * 1000);
    let deletedCount = 0;

    try {
        // 遍历客户端目录下的所有日期文件夹
        const dateDirs = fs.readdirSync(clientPath).filter(item => {
            const fullPath = path.join(clientPath, item);
            return fs.statSync(fullPath).isDirectory() && item !== 'config.json';
        });

        for (const dateDir of dateDirs) {
            const dateDirPath = path.join(clientPath, dateDir);
            const files = fs.readdirSync(dateDirPath);

            for (const file of files) {
                const filePath = path.join(dateDirPath, file);
                const fileStat = fs.statSync(filePath);

                // 如果文件时间早于截止时间，删除它
                if (fileStat.mtimeMs < cutoffTime) {
                    fs.unlinkSync(filePath);
                    deletedCount++;
                }
            }

            // 如果目录为空，删除目录
            const remainingFiles = fs.readdirSync(dateDirPath);
            if (remainingFiles.length === 0) {
                fs.rmdirSync(dateDirPath);
            }
        }

        logWithTime(`[DELETE] Deleted ${deletedCount} screenshot files older than ${hours} hours for client "${clientId}"`);

        return {
            success: true,
            deletedCount: deletedCount,
            message: `Deleted ${deletedCount} files older than ${hours} hours`
        };

    } catch (error) {
        errorWithTime(`[DELETE] Failed to delete screenshots for ${clientId}:`, error);
        throw error;
    }
}

/**
 * 删除客户端所有截图
 * @param {string} clientId - 客户端ID
 * @returns {object} 删除结果
 */
function deleteAllScreenshots(clientId) {
    const clientPath = path.join(config.UPLOADS_DIR, clientId);

    if (!fs.existsSync(clientPath)) {
        throw new Error('Client not found');
    }

    let deletedCount = 0;

    try {
        // 遍历客户端目录下的所有日期文件夹
        const dateDirs = fs.readdirSync(clientPath).filter(item => {
            const fullPath = path.join(clientPath, item);
            return fs.statSync(fullPath).isDirectory() && item !== 'config.json';
        });

        for (const dateDir of dateDirs) {
            const dateDirPath = path.join(clientPath, dateDir);
            const files = fs.readdirSync(dateDirPath);

            for (const file of files) {
                const filePath = path.join(dateDirPath, file);
                fs.unlinkSync(filePath);
                deletedCount++;
            }

            // 删除空文件夹
            fs.rmdirSync(dateDirPath);
        }

        logWithTime(`[DELETE ALL] Deleted all ${deletedCount} screenshot files for client "${clientId}"`);

        return {
            success: true,
            deletedCount: deletedCount,
            message: `Deleted all ${deletedCount} screenshot files`
        };

    } catch (error) {
        errorWithTime(`[DELETE ALL] Failed to delete all screenshots for ${clientId}:`, error);
        throw error;
    }
}

/**
 * 处理新截图上传
 * @param {object} file - 上传的文件对象
 * @param {string} clientId - 客户端ID
 * @param {string} alias - 客户端别名
 * @returns {string} 截图URL
 */
function processNewScreenshot(file, clientId, alias) {
    try {
        // 构建截图URL
        const pathAfterUploads = file.path.split('uploads')[1];
        const screenshotUrl = `/uploads${pathAfterUploads.replace(/\\/g, '/')}`;

        const displayName = alias ? `${alias} (${clientId})` : clientId;
        logWithTime('[CLIENT] Screenshot uploaded from:', displayName);

        return screenshotUrl;
    } catch (error) {
        errorWithTime('[CLIENT] Error processing screenshot:', error);
        throw error;
    }
}

module.exports = {
    getClientScreenshots,
    deleteScreenshotsByTime,
    deleteAllScreenshots,
    processNewScreenshot
};
