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
 * 更新客户端别名
 * PUT /web/clients/:currentAlias/alias
 */
router.put('/clients/:currentAlias/alias', async (req, res) => {
    try {
        const { currentAlias } = req.params;
        const { newAlias } = req.body;

        if (!newAlias || typeof newAlias !== 'string') {
            return res.status(400).json({
                success: false,
                message: '新别名不能为空'
            });
        }

        // 验证新别名格式（可以包含字母、数字、下划线、连字符）
        if (!/^[a-zA-Z0-9_-]+$/.test(newAlias)) {
            return res.status(400).json({
                success: false,
                message: '别名只能包含字母、数字、下划线和连字符'
            });
        }

        const result = await clientManager.updateAlias(currentAlias, newAlias);

        if (result.success) {
            res.json(result);
        } else {
            res.status(400).json(result);
        }
    } catch (error) {
        res.status(500).json({
            success: false,
            message: '更新别名时发生服务器错误'
        });
    }
});

/**
 * 删除客户端
 * DELETE /web/clients/:alias
 */
router.delete('/clients/:alias', async (req, res) => {
    try {
        const { alias } = req.params;

        const result = await clientManager.deleteClient(alias);

        if (result.success) {
            logWithTime(`[WEB] Client deleted: ${alias}`);
            res.json(result);
        } else {
            res.status(400).json(result);
        }
    } catch (error) {
        errorWithTime('[WEB] Error deleting client:', error);
        res.status(500).json({
            success: false,
            message: '删除客户端时发生服务器错误'
        });
    }
});

/**
 * 通用命令路由 - 支持所有命令类型
 * POST /web/command/:client_alias
 */
router.post('/command/:client_alias', (req, res) => {
    try {
        const clientAlias = req.params.client_alias;
        const { type, data } = req.body;

        if (!clientAlias || typeof clientAlias !== 'string' || clientAlias.trim() === '') {
            return res.status(400).json({
                success: false,
                message: 'Missing or invalid client alias'
            });
        }

        if (!type) {
            return res.status(400).json({
                success: false,
                message: 'Missing command type'
            });
        }

        if (!isClientOnline(clientAlias)) {
            return res.status(404).json({
                success: false,
                message: 'Client is not online'
            });
        }

        // 构建通用命令消息
        const message = {
            type: type,
            data: data || {}
        };        // 为某些命令类型添加默认的session_id
        if (['create_pty_session', 'pty_input', 'pty_resize', 'force_kill_session'].includes(type)) {
            if (!message.data.session_id) {
                message.data.session_id = clientAlias; // 使用client alias作为默认session id
            }
        }

        const sent = sendToClient(clientAlias, message);
        if (sent) {
            logWithTime(`[WEB] Command sent to "${clientAlias}": ${type}`);
            res.json({
                success: true,
                message: `Command ${type} sent to client`
            });
        } else {
            res.status(500).json({
                success: false,
                message: 'Failed to send command to client'
            });
        }
    } catch (error) {
        errorWithTime('[WEB] Error sending command:', error);
        res.status(500).json({
            success: false,
            message: 'Failed to send command'
        });
    }
});

module.exports = router;
