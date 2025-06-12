// 简单的Shell终端功能
// 处理shell命令的发送和输出显示

import { selectedClient } from './state.js';
import { showToast } from '../../toast/toast.js';

let terminalOutput = null;

/**
 * 初始化终端功能
 */
export function initTerminal() {
    terminalOutput = document.getElementById('terminalOutput');

    // 绑定终端按钮事件
    document.getElementById('cmdPwd')?.addEventListener('click', () => executeCommand('pwd'));
    document.getElementById('cmdLs')?.addEventListener('click', () => executeCommand('ls'));
    document.getElementById('cmdDir')?.addEventListener('click', () => executeCommand('dir'));
    document.getElementById('cmdClear')?.addEventListener('click', clearTerminal);

    // 绑定自定义命令执行
    const executeBtn = document.getElementById('executeCustom');
    const customInput = document.getElementById('customCommand');

    executeBtn?.addEventListener('click', executeCustomCommand);
    customInput?.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            executeCustomCommand();
        }
    });
}

/**
 * 更新终端状态（启用/禁用控件）
 * @param {boolean} enabled - 是否启用
 */
export function updateTerminalState(enabled) {
    const buttons = ['cmdPwd', 'cmdLs', 'cmdDir', 'executeCustom'];
    const customInput = document.getElementById('customCommand');
    const terminalMessage = document.getElementById('terminalMessage');

    buttons.forEach(id => {
        const button = document.getElementById(id);
        if (button) {
            button.disabled = !enabled;
        }
    });

    if (customInput) {
        customInput.disabled = !enabled;
    }

    if (terminalMessage) {
        terminalMessage.style.display = enabled ? 'none' : 'block';
    }

    if (!enabled && terminalOutput) {
        terminalOutput.textContent = '[Terminal Ready]\n等待选择客户端...';
    }
}

/**
 * 执行shell命令
 * @param {string} command - 要执行的命令
 */
async function executeCommand(command) {
    if (!selectedClient) {
        showToast('请先选择一个客户端');
        return;
    }

    if (!command || command.trim() === '') {
        showToast('命令不能为空');
        return;
    }

    addToTerminal(`> ${command}`);

    try {
        const response = await fetch(`/web/command/${selectedClient}`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                type: 'shell_execute',
                data: {
                    command: command.trim()
                }
            })
        });

        const result = await response.json();

        if (!result.success) {
            addToTerminal(`错误: ${result.message}`);
            showToast(`命令执行失败: ${result.message}`);
        }
        // 成功时不需要立即显示，等待WebSocket返回结果

    } catch (error) {
        console.error('发送命令失败:', error);
        addToTerminal(`错误: ${error.message}`);
        showToast('发送命令失败');
    }
}

/**
 * 执行自定义命令
 */
function executeCustomCommand() {
    const customInput = document.getElementById('customCommand');
    if (!customInput) return;

    const command = customInput.value.trim();
    if (command) {
        executeCommand(command);
        customInput.value = ''; // 清空输入框
    }
}

/**
 * 清空终端输出
 */
function clearTerminal() {
    if (terminalOutput) {
        terminalOutput.textContent = '[Terminal Ready]\n';
    }
}

/**
 * 添加文本到终端输出
 * @param {string} text - 要添加的文本
 */
function addToTerminal(text) {
    if (!terminalOutput) {
        return;
    }

    terminalOutput.textContent += text + '\n';
    // 自动滚动到底部
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
}

/**
 * 处理从WebSocket接收到的shell输出
 * @param {string} output - shell命令的输出
 */
export function handleShellOutput(output) {
    if (!output) {
        return;
    }

    // 清理ANSI转义序列
    const cleanOutput = output.replace(/\x1b\[[0-9;]*m/g, '');
    addToTerminal(cleanOutput);
}
