const fs = require('fs');
const path = require('path');
const { getClients } = require('./clients');

const commandFile = path.join(__dirname, 'commandQueue.json');

// 获取并清空指令（一次性读取）
function getAndClearCommands(clientId) {
    if (!fs.existsSync(commandFile)) return [];

    //解析json
    const allCommands = JSON.parse(fs.readFileSync(commandFile, 'utf-8'));
    
    //获取用户id或别名作为key，别名优先
    const clients = getClients();
    const alias = clients[clientId];
    const key = alias || clientId;

    //获取命令
    const clientCommands = allCommands[key] || [];

    // 清空该用户的命令队列
    delete allCommands[key];
    fs.writeFileSync(commandFile, JSON.stringify(allCommands, null, 2));

    return clientCommands;
}

// 添加命令（供其他接口或测试用）
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
    addCommand
};
