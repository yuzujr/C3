// 简单的Shell终端功能
// 处理shell命令的发送和输出显示

import { selectedClient } from './state.js';
import { showToast } from '../../toast/toast.js';
import { sendCommand } from './commands.js';

let terminalOutput = null;
let currentWorkingDirectory = '~'; // 跟踪当前工作目录

/**
 * 生成带有当前目录的提示符
 * @param {string} client - 客户端名称
 * @returns {string} 格式化的提示符
 */
function generatePrompt(client) {
    return `\x1b[32m${client}\x1b[0m \x1b[34m[${currentWorkingDirectory}]\x1b[0m \x1b[36m>\x1b[0m `;
}

/**
 * 生成彩色HTML提示符用于显示
 * @param {string} client - 客户端名称
 * @returns {string} HTML格式的提示符
 */
function generateHtmlPrompt(client) {
    return `<span style="color: var(--terminal-success); font-weight: 600;">${client}</span> <span style="color: var(--terminal-command);">[${currentWorkingDirectory}]</span> <span style="color: var(--terminal-prompt); font-weight: 600;">></span> `;
}

/**
 * 重置当前工作目录（当切换客户端时调用）
 */
export function resetWorkingDirectory() {
    currentWorkingDirectory = '~';

    // 清空输入框
    const customInput = document.getElementById('customCommand');
    if (customInput) {
        customInput.value = '';
    }

    // 清空终端内容
    if (terminalOutput) {
        terminalOutput.innerHTML = '';
    }

    // 为新客户端显示初始提示符
    if (selectedClient) {
        showNextPrompt();
    }
}

/**
 * 显示下一个命令提示符，并同步输入框内容
 */
function showNextPrompt() {
    if (!selectedClient) return;

    const customInput = document.getElementById('customCommand');
    const inputText = customInput ? customInput.value : ''; const promptHtml = generateHtmlPrompt(selectedClient) +
        `<span style="color: var(--terminal-command); font-weight: 600;">${inputText}</span>`;

    addHtmlToTerminal(promptHtml, true); // 标记为提示符
}

/**
 * 移除当前的提示符行（如果存在）
 */
function removeCurrentPrompt() {
    if (!selectedClient || !terminalOutput) return;

    // 检查最后一个子元素是否是提示符
    const lastElement = terminalOutput.lastElementChild;
    if (lastElement && lastElement.classList.contains('terminal-prompt')) {
        lastElement.remove();
    }
}

/**
 * 更新提示符显示（当输入框内容变化时调用）
 */
function updatePromptDisplay() {
    if (!selectedClient || !terminalOutput) return;

    // 检查最后一个元素是否是提示符
    const lastElement = terminalOutput.lastElementChild;
    if (lastElement && lastElement.classList.contains('terminal-prompt')) {
        // 更新提示符内容
        const customInput = document.getElementById('customCommand');
        const inputText = customInput ? customInput.value : ''; const promptHtml = generateHtmlPrompt(selectedClient) +
            `<span style="color: var(--terminal-command); font-weight: 600;">${inputText}</span>`;

        lastElement.innerHTML = promptHtml;
    }
}

/**
 * 初始化终端功能
 */
export function initTerminal() {
    terminalOutput = document.getElementById('terminalOutput');    // 绑定终端按钮事件
    document.getElementById('cmdPwd')?.addEventListener('click', () => executeQuickCommand('pwd'));
    document.getElementById('cmdLs')?.addEventListener('click', () => executeQuickCommand('ls'));
    document.getElementById('cmdClear')?.addEventListener('click', clearTerminal);
    document.getElementById('cmdKillSession')?.addEventListener('click', killSession);

    // 绑定自定义命令执行
    const executeBtn = document.getElementById('executeCustom');
    const customInput = document.getElementById('customCommand');

    executeBtn?.addEventListener('click', executeCustomCommand);
    customInput?.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            executeCustomCommand();
        }
    });    // 监听输入框内容变化，实时更新提示符显示
    customInput?.addEventListener('input', updatePromptDisplay);
}

/**
 * 更新终端状态（启用/禁用控件）
 * @param {boolean} enabled - 是否启用
 */
export function updateTerminalState(enabled) {
    const buttons = ['cmdPwd', 'cmdLs', 'executeCustom', 'cmdKillSession'];
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
    } if (terminalMessage) {
        if (enabled) {
            terminalMessage.textContent = '终端已就绪';
            terminalMessage.style.display = 'none';
            // 当终端启用时，只有在终端为空或没有正确的提示符时才显示初始提示符
            if (terminalOutput && selectedClient) {
                const content = terminalOutput.textContent.trim();
                const lines = content.split('\n').filter(line => line.trim() !== '');
                const lastLine = lines.length > 0 ? lines[lines.length - 1] : '';
                const promptPrefix = generatePrompt(selectedClient);

                // 只有在以下情况下才显示新的提示符：
                // 1. 终端完全为空
                // 2. 最后一行不是当前客户端的提示符
                const needsPrompt = content === '' ||
                    (lastLine !== '' && !lastLine.startsWith(promptPrefix.trim()));

                if (needsPrompt) {
                    showNextPrompt();
                }
            }
        } else {
            terminalMessage.textContent = '请选择一个在线客户端';
            terminalMessage.style.display = 'block';
        }
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

    try {
        const result = await sendCommand({
            type: 'shell_execute',
            data: {
                command: command.trim()
            }
        }, false); // 不显示默认toast，我们自己处理

        if (!result) {
            addToTerminal(`错误: 发送命令失败`);
            showToast('发送命令失败');
            showNextPrompt();
            return;
        }

        if (!result.success) {
            addToTerminal(`错误: ${result.message}`);
            showToast(`命令执行失败: ${result.message}`);
            showNextPrompt();
        }
        // 成功时不需要立即显示，等待WebSocket返回结果

    } catch (error) {
        console.error('发送命令失败:', error);
        addToTerminal(`错误: ${error.message}`);
        showToast('发送命令失败');
        showNextPrompt();
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
        // 移除当前的空提示符（如果有的话）
        removeCurrentPrompt();        // 显示带命令的提示符
        const promptHtml = generateHtmlPrompt(selectedClient) +
            `<span style="color: var(--terminal-command); font-weight: 600;">${command}</span>`;
        addHtmlToTerminal(promptHtml, true); // 标记为提示符

        // 清空输入框
        customInput.value = '';

        // 执行命令
        executeCommand(command);
    }
}

/**
 * 清空终端输出
 */
function clearTerminal() {
    if (terminalOutput) {
        terminalOutput.innerHTML = '';
        // 清空后显示新的提示符
        if (selectedClient) {
            showNextPrompt();
        }
    }
}

/**
 * 强制终止会话
 */
async function killSession() {
    if (!selectedClient) {
        showToast('请先选择一个客户端');
        return;
    }

    // 确认对话框
    if (!confirm('确定要强制终止当前会话吗？这将关闭所有正在运行的程序并重新创建会话。')) {
        return;
    }

    try {
        const result = await sendCommand({
            type: 'force_kill_session',
            data: {
                session_id: selectedClient
            }
        }, false); // 不显示默认toast

        if (!result) {
            addErrorToTerminal('强制终止失败: 发送请求失败');
            showToast('强制终止会话失败');
            return;
        } if (result.success) {
            addSuccessToTerminal('[会话已强制终止]');
            showToast('会话已强制终止');
            // 不需要手动显示提示符，handleShellOutput 会处理
        } else {
            addErrorToTerminal(`强制终止失败: ${result.message}`);
            showToast(`强制终止失败: ${result.message}`);
        }
    } catch (error) {
        console.error('强制终止会话失败:', error);
        addErrorToTerminal(`强制终止失败: ${error.message}`);
        showToast('强制终止会话失败');
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

    // 如果终端内容为空或只包含空白字符，先清空它
    if (terminalOutput.textContent.trim() === '') {
        terminalOutput.textContent = '';
    }

    terminalOutput.textContent += text + '\n';
    // 自动滚动到底部
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
}

/**
 * 添加HTML格式的文本到终端输出
 * @param {string} htmlContent - 要添加的HTML内容
 * @param {boolean} isPrompt - 是否是提示符
 */
function addHtmlToTerminal(htmlContent, isPrompt = false) {
    if (!terminalOutput) {
        return;
    }

    // 创建新的div元素来包含这一行
    const lineDiv = document.createElement('div');
    lineDiv.innerHTML = htmlContent;
    lineDiv.style.marginBottom = '0';
    lineDiv.style.lineHeight = '1.5';

    // 如果是提示符，添加特殊的类名
    if (isPrompt) {
        lineDiv.classList.add('terminal-prompt');
    }

    terminalOutput.appendChild(lineDiv);

    // 自动滚动到底部
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
}

/**
 * 清理ANSI转义序列并转换为HTML颜色
 * @param {string} text - 包含ANSI转义序列的文本
 * @returns {string} HTML格式的彩色文本
 */
function ansiToHtml(text) {
    // 简单的ANSI到HTML颜色映射
    const colorMap = {
        '30': 'color: #5c6370', // 黑色 -> 注释色
        '31': 'color: #e06c75', // 红色 -> 错误色
        '32': 'color: #98c379', // 绿色 -> 成功色
        '33': 'color: #e5c07b', // 黄色 -> 警告色
        '34': 'color: #61afef', // 蓝色 -> 命令色
        '35': 'color: #c678dd', // 紫色
        '36': 'color: #56b6c2', // 青色 -> 提示符色
        '37': 'color: #abb2bf', // 白色 -> 默认文本色
    };

    // 替换ANSI转义序列
    return text
        .replace(/\x1b\[(\d+)m/g, (match, code) => {
            if (code === '0') return '</span>'; // 重置
            if (colorMap[code]) return `<span style="${colorMap[code]}; font-weight: inherit;">`;
            return '';
        })
        .replace(/\x1b\[0m/g, '</span>');
}

/**
 * 处理从WebSocket接收到的shell输出
 * @param {object|string} data - shell命令的输出数据或字符串
 */
export function handleShellOutput(data) {
    if (!data) {
        return;
    }

    let output, cwd;

    // 支持两种格式：字符串（向后兼容）或对象
    if (typeof data === 'string') {
        output = data;
        cwd = '';
    } else {
        output = data.output || data.stdout || '';
        cwd = data.cwd || '';
    }    // 更新当前工作目录
    if (cwd && cwd.trim()) {
        currentWorkingDirectory = cwd.trim();
    }

    // 清理ANSI转义序列
    const cleanOutput = output.replace(/\x1b\[[0-9;]*m/g, '');

    // 显示命令输出
    if (cleanOutput.trim()) {
        const processedOutput = processTerminalOutput(cleanOutput);
        addHtmlToTerminal(processedOutput);
    }    // 命令执行完成后，显示新的提示符
    showNextPrompt();
}

/**
 * 执行快捷命令（先显示在提示符后，然后执行）
 * @param {string} command - 要执行的命令
 */
async function executeQuickCommand(command) {
    if (!selectedClient) {
        showToast('请先选择一个客户端');
        return;
    }

    if (!command || command.trim() === '') {
        showToast('命令不能为空');
        return;
    }    // 清空输入框（如果有内容的话）
    const customInput = document.getElementById('customCommand');
    if (customInput) {
        customInput.value = '';
    }

    // 移除当前的空提示符（如果有的话）
    removeCurrentPrompt();

    // 显示带命令的提示符
    const promptHtml = generateHtmlPrompt(selectedClient) +
        `<span style="color: var(--terminal-command); font-weight: 600;">${command}</span>`;
    addHtmlToTerminal(promptHtml, true); // 标记为提示符

    // 执行命令，但不在出错时显示提示符，因为我们已经显示了
    try {
        const result = await sendCommand({
            type: 'shell_execute',
            data: {
                command: command.trim()
            }
        }, false); // 不显示默认toast

        if (!result) {
            addToTerminal(`错误: 发送命令失败`);
            showToast('发送命令失败');
            showNextPrompt();
            return;
        }

        if (!result.success) {
            addToTerminal(`错误: ${result.message}`);
            showToast(`命令执行失败: ${result.message}`);
            showNextPrompt();
        }
        // 成功时不需要立即显示，等待WebSocket返回结果

    } catch (error) {
        console.error('发送命令失败:', error);
        addToTerminal(`错误: ${error.message}`);
        showToast('发送命令失败');
        showNextPrompt();
    }
}

/**
 * 添加彩色错误消息到终端
 * @param {string} message - 错误消息
 */
function addErrorToTerminal(message) {
    addHtmlToTerminal(`<span style="color: var(--terminal-error);">❌ ${message}</span>`);
}

/**
 * 添加彩色成功消息到终端
 * @param {string} message - 成功消息
 */
function addSuccessToTerminal(message) {
    addHtmlToTerminal(`<span style="color: var(--terminal-success);">✅ ${message}</span>`);
}

/**
 * 添加彩色警告消息到终端
 * @param {string} message - 警告消息
 */
function addWarningToTerminal(message) {
    addHtmlToTerminal(`<span style="color: var(--terminal-warning);">⚠️ ${message}</span>`);
}

/**
 * 处理终端输出的清理和着色
 * @param {string} output - 原始输出
 * @returns {string} 处理后的HTML输出
 */
function processTerminalOutput(output) {
    // 清理ANSI转义序列
    const cleanOutput = output.replace(/\x1b\[[0-9;]*m/g, '');

    // 基本的语法高亮
    let processedOutput = cleanOutput
        // 突出显示文件路径
        .replace(/([\/\w\-\.]+\.[a-zA-Z]{1,4})/g, '<span style="color: var(--terminal-command);">$1</span>')
        // 突出显示目录路径
        .replace(/([\/\w\-\.]+\/)/g, '<span style="color: var(--terminal-prompt);">$1</span>')
        // 突出显示数字
        .replace(/\b(\d+)\b/g, '<span style="color: var(--terminal-warning);">$1</span>');

    return processedOutput;
}
