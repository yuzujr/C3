// Web接口路由模块
// 处理所有面向Web界面的API路由

const express = require('express');
const fs = require('fs');
const path = require('path');
const config = require('../config');
const { logWithTime, errorWithTime } = require('../logger');
const { getClientScreenshots, deleteScreenshotsByTime, deleteAllScreenshots } = require('../services/screenshot');
const { getClientConfig } = require('../services/command');
const { sendToClient, isClientOnline } = require('../websocket');
const { getClients } = require('../clients');

const router = express.Router();

/**
 * 获取客户端列表
 */
router.get('/clients', (req, res) => {
    try {
        if (!fs.existsSync(config.UPLOADS_DIR)) {
            return res.json([]);
        }

        const clients = fs.readdirSync(config.UPLOADS_DIR).filter(item => {
            const fullPath = path.join(config.UPLOADS_DIR, item);
            return fs.statSync(fullPath).isDirectory();
        });

        res.json(clients);
    } catch (error) {
        errorWithTime('[WEB] Error getting clients:', error);
        res.status(500).send('Failed to get clients');
    }
});

/**
 * 获取客户端截图列表
 */
router.get('/screenshots/:client_id', (req, res) => {
    try {
        const clientId = req.params.client_id;
        const sinceTimestamp = req.query.since ? parseInt(req.query.since) : 0;

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).send('Missing or invalid client_id');
        }

        const screenshots = getClientScreenshots(clientId, sinceTimestamp);
        res.json(screenshots);
    } catch (error) {
        errorWithTime('[WEB] Error getting screenshots:', error);
        res.status(500).send('Failed to get screenshots');
    }
});

/**
 * 获取客户端配置
 */
router.get('/config/:clientId', (req, res) => {
    try {
        const clientId = req.params.clientId;

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).send('Missing or invalid clientId');
        }

        const config = getClientConfig(clientId);
        if (!config) {
            return res.status(404).send('Client config not found');
        }

        res.json(config);
    } catch (error) {
        errorWithTime('[WEB] Error getting config:', error);
        res.status(500).send('Failed to read config');
    }
});

/**
 * 向客户端发送命令
 */
router.post('/send_commands', (req, res) => {
    try {
        const { client_id, command } = req.body;

        if (!client_id || !command) {
            return res.status(400).send('Missing client_id or command');
        }

        const clients = getClients();
        const alias = clients[client_id] || client_id;

        // 如果客户端在线，直接推送命令
        if (isClientOnline(alias)) {
            const success = sendToClient(alias, { commands: [command] });
            if (success) {
                logWithTime(`[WEB] Command sent to "${alias}":`, command.type || 'unknown');
                res.sendStatus(200);
            } else {
                res.status(503).send(`Failed to send command to "${alias}"`);
            }
        } else {
            res.status(503).send(`Client "${alias}" is not online`);
        }
    } catch (error) {
        errorWithTime('[WEB] Error sending command:', error);
        res.status(500).send('Failed to send command');
    }
});

/**
 * 删除指定时间范围内的截图
 */
router.delete('/delete-screenshots/:client_id', (req, res) => {
    try {
        const clientId = req.params.client_id;
        const { hours } = req.body;

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).send('Missing or invalid client_id');
        }

        if (!hours || typeof hours !== 'number' || hours <= 0) {
            return res.status(400).send('Missing or invalid hours parameter');
        }

        const result = deleteScreenshotsByTime(clientId, hours);
        res.json(result);
    } catch (error) {
        if (error.message === 'Client not found') {
            res.status(404).send('Client not found');
        } else {
            errorWithTime('[WEB] Error deleting screenshots:', error);
            res.status(500).send('Failed to delete screenshots: ' + error.message);
        }
    }
});

/**
 * 删除客户端所有截图
 */
router.delete('/delete-all-screenshots/:client_id', (req, res) => {
    try {
        const clientId = req.params.client_id;

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).send('Missing or invalid client_id');
        }

        const result = deleteAllScreenshots(clientId);
        res.json(result);
    } catch (error) {
        if (error.message === 'Client not found') {
            res.status(404).send('Client not found');
        } else {
            errorWithTime('[WEB] Error deleting all screenshots:', error);
            res.status(500).send('Failed to delete all screenshots: ' + error.message);
        }
    }
});

module.exports = router;
