// Web接口路由模块
// 处理所有面向Web界面的API路由

const express = require('express');
const { logWithTime, errorWithTime } = require('../logger');
const { getClientScreenshots, deleteScreenshotsByTime, deleteAllScreenshots } = require('../services/screenshot-service');
const { getClientConfig } = require('../services/command-service');
const { sendToClient, isClientOnline } = require('../websocket');
const clientService = require('../services/client-service');

const router = express.Router();

/**
 * 获取客户端列表
 * 返回格式: [{ client_id: "xxx", alias: "test", online: true }]
 */
router.get('/clients', async (req, res) => {
    try {
        const { getOnlineClients } = require('../websocket');
        const allClients = await clientService.getAllClients();
        const onlineClientIds = getOnlineClients(); // 现在返回 client_id 列表

        // 构建客户端列表 - 包含client_id和alias
        const clientsList = allClients.map(client => ({
            client_id: client.client_id,
            alias: client.alias,
            online: onlineClientIds.includes(client.client_id)
        }));

        // 添加在线但可能不在数据库中的客户端
        onlineClientIds.forEach(client_id => {
            const existingClient = clientsList.find(c => c.client_id === client_id);
            if (!existingClient) {
                clientsList.push({
                    client_id: client_id,
                    alias: client_id, // 为在线但未注册的客户端，使用client_id作为临时alias
                    online: true
                });
            }
        });

        // 添加缓存头，减少前端轮询压力
        res.set({
            'Cache-Control': 'private, max-age=5', // 5秒缓存
            'ETag': `"${Date.now()}"` // 简单的ETag
        });

        res.json(clientsList);
    } catch (error) {
        errorWithTime('[WEB] Error getting clients:', error);
        res.status(500).json({ error: 'Failed to get clients', details: error.message });
    }
});

/**
 * 获取客户端截图列表
 */
router.get('/screenshots/:client_id', async (req, res) => {
    try {
        const clientId = req.params.client_id;
        const sinceTimestamp = req.query.since ? parseInt(req.query.since) : 0;

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).send('Missing or invalid client_id');
        }

        // 直接使用 client_id 获取截图，因为现在截图目录使用 client_id
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
router.get('/config/:client_id', async (req, res) => {
    try {
        const clientId = req.params.client_id; // 使用 client_id

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).send('Missing or invalid client_id');
        }

        const config = await getClientConfig(clientId);
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
router.delete('/delete-screenshots/:client_id', async (req, res) => {
    try {
        const clientId = req.params.client_id;
        const { hours } = req.body;

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).send('Missing or invalid client_id');
        }

        if (!hours || typeof hours !== 'number' || hours <= 0) {
            return res.status(400).send('Missing or invalid hours parameter');
        }

        // 直接使用 client_id 查找截图目录
        const result = deleteScreenshotsByTime(clientId, hours);
        res.json(result);
    } catch (error) {
        if (error.message === 'Client not found') {
            res.status(404).json({ success: false, message: 'Client not found' });
        } else {
            errorWithTime('[WEB] Error deleting screenshots:', error);
            res.status(500).json({ success: false, message: 'Failed to delete screenshots: ' + error.message });
        }
    }
});

/**
 * 删除客户端所有截图
 */
router.delete('/delete-all-screenshots/:client_id', async (req, res) => {
    try {
        const clientId = req.params.client_id;

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).send('Missing or invalid client_id');
        }

        // 直接使用 client_id 查找截图目录
        const result = deleteAllScreenshots(clientId);
        res.json(result);
    } catch (error) {
        if (error.message === 'Client not found') {
            res.status(404).json({ success: false, message: 'Client not found' });
        } else {
            errorWithTime('[WEB] Error deleting all screenshots:', error);
            res.status(500).json({ success: false, message: 'Failed to delete all screenshots: ' + error.message });
        }
    }
});

/**
 * 更新客户端别名
 * PUT /web/clients/:client_id/alias
 */
router.put('/clients/:client_id/alias', async (req, res) => {
    try {
        const clientId = req.params.client_id;
        const { newAlias } = req.body;

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).json({
                success: false,
                message: 'Missing or invalid client_id'
            });
        }

        if (!newAlias || typeof newAlias !== 'string' || newAlias.trim() === '') {
            return res.status(400).json({
                success: false,
                message: 'Missing or invalid newAlias'
            });
        }

        // 验证新别名格式（可以包含字母、数字、下划线、连字符）
        if (!/^[a-zA-Z0-9_-]+$/.test(newAlias)) {
            return res.status(400).json({
                success: false,
                message: '别名只能包含字母、数字、下划线和连字符'
            });
        }

        // 获取当前别名用于广播事件
        const currentAlias = await clientService.getAlias(clientId);

        // 直接使用 client_id 更新别名
        const success = await clientService.updateClientAlias(clientId, newAlias);

        if (success) {
            // 通过WebSocket广播别名更新事件
            try {
                const { broadcastToWebClients } = require('../websocket');
                broadcastToWebClients({
                    type: 'alias_updated',
                    client_id: clientId,
                    oldAlias: currentAlias,
                    newAlias: newAlias
                });
            } catch (error) {
                errorWithTime('[WEB] Failed to broadcast alias update:', error);
            }
            
            res.json({
                success: true,
                message: '别名更新成功',
                oldAlias: currentAlias,
                newAlias: newAlias
            });
        } else {
            res.status(400).json({
                success: false,
                message: '更新别名失败'
            });
        }
    } catch (error) {
        errorWithTime('[WEB] Error updating alias:', error);
        res.status(500).json({
            success: false,
            message: '更新别名时发生服务器错误'
        });
    }
});

/**
 * 删除客户端
 * DELETE /web/clients/:client_id
 */
router.delete('/clients/:client_id', async (req, res) => {
    try {
        const clientId = req.params.client_id;

        const result = await clientService.deleteClient(clientId);

        if (result.success) {
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
 * POST /web/command/:client_id
 */
router.post('/command/:client_id', async (req, res) => {
    try {
        const clientId = req.params.client_id;
        const { type, data } = req.body;

        if (!clientId || typeof clientId !== 'string' || clientId.trim() === '') {
            return res.status(400).json({
                success: false,
                message: 'Missing or invalid client_id'
            });
        }

        if (!type) {
            return res.status(400).json({
                success: false,
                message: 'Missing command type'
            });
        }

        // 直接使用 client_id 检查在线状态
        if (!isClientOnline(clientId)) {
            return res.status(404).json({
                success: false,
                message: 'Client is not online'
            });
        }

        // 构建通用命令消息
        const message = {
            type: type,
            data: data || {}
        };
        
        // 为某些命令类型添加默认的session_id
        if (['create_pty_session', 'pty_input', 'pty_resize', 'force_kill_session'].includes(type)) {
            if (!message.data.session_id) {
                // 使用 client_id 作为默认 session id
                message.data.session_id = clientId;
            }
        }

        const sent = sendToClient(clientId, message); // 直接使用 client_id
        if (sent) {
            // 抑制高频无用日志输出
            const noisyCommands = ['pty_input', 'pty_resize'];
            if (!noisyCommands.includes(type)) {
                logWithTime(`[WEB] Command sent to "${clientId}": ${type}`);
            }
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
