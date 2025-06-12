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
const clientManager = require('../client-manager');

const router = express.Router();

/**
 * 获取客户端列表
 * 返回格式: [{ alias: "test", online: true }]
 */
router.get('/clients', (req, res) => {
    try {
        const { getOnlineClients } = require('../websocket');
        const allClients = clientManager.getAllClients();
        const onlineAliases = getOnlineClients();

        // 构建简化的客户端列表 - 只使用alias作为标识符
        const clientsList = allClients.map(client => ({
            alias: client.alias,
            online: onlineAliases.includes(client.alias)
        }));

        // 添加在线但可能不在clients.json中的客户端
        onlineAliases.forEach(alias => {
            const existingClient = clientsList.find(c => c.alias === alias);
            if (!existingClient) {
                clientsList.push({
                    alias: alias,
                    online: true
                });
            }
        });

        res.json(clientsList);
    } catch (error) {
        errorWithTime('[WEB] Error getting clients:', error);
        res.status(500).send('Failed to get clients');
    }
});

/**
 * 获取客户端截图列表
 */
router.get('/screenshots/:client_alias', (req, res) => {
    try {
        const clientAlias = req.params.client_alias;
        const sinceTimestamp = req.query.since ? parseInt(req.query.since) : 0;

        if (!clientAlias || typeof clientAlias !== 'string' || clientAlias.trim() === '') {
            return res.status(400).send('Missing or invalid client_alias');
        }

        // 使用alias直接获取截图
        const screenshots = getClientScreenshots(clientAlias, sinceTimestamp);
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
        const clientAlias = req.params.clientId; // 现在参数实际上是alias

        if (!clientAlias || typeof clientAlias !== 'string' || clientAlias.trim() === '') {
            return res.status(400).send('Missing or invalid client alias');
        }

        const config = getClientConfig(clientAlias);
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
 * 删除指定时间范围内的截图
 */
router.delete('/delete-screenshots/:client_alias', (req, res) => {
    try {
        const clientAlias = req.params.client_alias;
        const { hours } = req.body;

        if (!clientAlias || typeof clientAlias !== 'string' || clientAlias.trim() === '') {
            return res.status(400).send('Missing or invalid client_alias');
        }

        if (!hours || typeof hours !== 'number' || hours <= 0) {
            return res.status(400).send('Missing or invalid hours parameter');
        }

        const result = deleteScreenshotsByTime(clientAlias, hours);
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
router.delete('/delete-all-screenshots/:client_alias', (req, res) => {
    try {
        const clientAlias = req.params.client_alias;

        if (!clientAlias || typeof clientAlias !== 'string' || clientAlias.trim() === '') {
            return res.status(400).send('Missing or invalid client_alias');
        }

        const result = deleteAllScreenshots(clientAlias);
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

/**
 * 发送命令到指定客户端 (新的API路由)
 */
router.post('/command/:client_alias', (req, res) => {
    try {
        const clientAlias = req.params.client_alias;
        const command = req.body;

        if (!clientAlias || !command) {
            return res.status(400).json({
                success: false,
                message: 'Missing client_alias or command'
            });
        }

        // 检查客户端是否在线
        if (!isClientOnline(clientAlias)) {
            return res.status(503).json({
                success: false,
                message: `Client "${clientAlias}" is not online`
            });
        }        // 发送命令 - 包装为commands数组格式以保持与客户端的兼容性
        const success = sendToClient(clientAlias, { commands: [command] });
        if (success) {
            logWithTime(`[WEB] Command sent to "${clientAlias}":`, command.type || 'unknown');
            res.json({
                success: true,
                message: 'Command sent successfully'
            });
        } else {
            res.status(503).json({
                success: false,
                message: `Failed to send command to "${clientAlias}"`
            });
        }
    } catch (error) {
        errorWithTime('[WEB] Error sending command:', error);
        res.status(500).json({
            success: false,
            message: 'Internal server error'
        });
    }
});

module.exports = router;
