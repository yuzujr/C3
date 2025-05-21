const fs = require('fs');
const path = require('path');
const { getClients } = require('./clients');

const commandFile = path.join(__dirname, 'commandQueue.json');

// 获取并清空指令
function getAndClearCommands(clientId) {
    if (!fs.existsSync(commandFile)) return [];

    // 读取并检查内容是否为空或仅空白
    let fileContent = fs.readFileSync(commandFile, 'utf-8').trim();
    if (!fileContent) {
        fileContent = '{}';  // 用空对象替代
    }

    let allCommands;
    try {
        allCommands = JSON.parse(fileContent);
    } catch (e) {
        console.error('[COMMAND] JSON 解析失败，内容可能损坏：', e);
        return [];
    }

    // 获取用户id或别名作为key，别名优先
    const clients = getClients();
    const alias = clients[clientId];
    const key = alias || clientId;

    // 获取命令
    const clientCommands = allCommands[key] || [];

    // 清空该用户的命令队列
    delete allCommands[key];
    fs.writeFileSync(commandFile, JSON.stringify(allCommands, null, 2));

    return clientCommands;
}

// 清空指定客户命令
function clearClientCommands(clientId) {
    if (!fs.existsSync(commandFile)) return;

    let allCommands = JSON.parse(fs.readFileSync(commandFile, 'utf-8'));

    // 清空该用户的命令队列
    delete allCommands[clientId];
    fs.writeFileSync(commandFile, JSON.stringify(allCommands, null, 2));
}

// 添加命令
function addCommand(clientId, command) {
    let allCommands = {};
    if (fs.existsSync(commandFile)) {
        allCommands = JSON.parse(fs.readFileSync(commandFile, 'utf-8'));
    }

    if (!allCommands[clientId]) allCommands[clientId] = [];
    allCommands[clientId].push(command);

    fs.writeFileSync(commandFile, JSON.stringify(allCommands, null, 2));
}

module.exports = {
    getAndClearCommands,
    addCommand,
    clearClientCommands
};
