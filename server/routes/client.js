// 客户端接口路由模块
// 处理所有面向客户端的API路由

const express = require('express');
const { upload, handleUploadError } = require('../upload');
const { logWithTime, errorWithTime } = require('../logger');
const clientService = require('../services/client-service');
const { processNewScreenshot } = require('../services/screenshot-service');
const { handleConfigUpdate } = require('../services/command-service');
const { broadcastToWebClients } = require('../websocket');

const router = express.Router();

/**
 * 客户端上传截图
 */
router.post('/upload_screenshot', upload.single('file'), async (req, res) => {
    try {
        const clientId = req.clientId;

        if (!clientId) {
            return res.status(400).send('Missing client_id parameter');
        }

        if (!req.file) {
            errorWithTime('[UPLOAD] No file received.');
            return res.status(400).send('No file received.');
        }

        const alias = await clientService.getAlias(clientId);

        // 处理新截图
        const screenshotUrl = processNewScreenshot(req.file, clientId, alias);

        // 记录截图到数据库
        await clientService.logScreenshot(
            clientId, 
            req.file.filename, 
            req.file.path, 
            req.file.size
        );

        // 向所有连接的 Web 客户端广播新截图通知
        broadcastToWebClients({
            type: 'new_screenshot',
            client_id: clientId,
            screenshot_url: screenshotUrl,
            timestamp: Date.now()
        });

        return res.status(201).send('Screenshot saved successfully: ' + req.file.filename);
    } catch (error) {
        errorWithTime('[UPLOAD] Error uploading screenshot:', error);
        res.status(500).send('Failed to upload screenshot: ' + error.message);
    }
});

/**
 * 客户端上传配置
 */
router.post('/upload_client_config', async (req, res) => {
    try {
        const clientId = req.query.client_id;
        const configData = req.body;

        if (!clientId) {
            return res.status(400).send('Missing client_id parameter');
        }

        // 更新客户端信息到数据库，保持现有别名
        const existingAlias = await clientService.getAlias(clientId);
        await clientService.setClient(clientId, existingAlias || clientId, {
            hostname: configData.hostname,
            ip_address: req.ip,
            platform: configData.platform || 'unknown'
        });

        const result = await handleConfigUpdate(clientId, configData);

        if (result.success) {
            res.status(200).send(result.message);
        } else {
            res.status(result.status || 500).send(result.error);
        }
    } catch (error) {
        errorWithTime('[CLIENT] Error uploading config:', error);
        res.status(500).send('Failed to upload config: ' + error.message);
    }
});

/**
 * 客户端心跳检测
 */
router.get('/heartbeat', async (req, res) => {
    const clientId = req.clientId;

    if (clientId) {
        logWithTime(`[HEARTBEAT] Client ${clientId} is alive`);
        
        // 更新客户端最后活跃时间
        try {
            await clientService.setClientOnline(clientId);
        } catch (error) {
            errorWithTime('[HEARTBEAT] Failed to update client status:', error);
        }
    }

    res.json({
        status: 'ok',
        timestamp: Date.now(),
        server_time: new Date().toISOString()
    });
});

/**
 * 获取服务器状态
 */
router.get('/status', (req, res) => {
    res.json({
        status: 'running',
        version: '1.6.0',
        uptime: process.uptime(),
        memory: process.memoryUsage(),
        timestamp: Date.now()
    });
});

// 错误处理中间件
router.use(handleUploadError);

module.exports = router;
