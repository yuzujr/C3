#ifndef TERMINAL_MANAGER_H
#define TERMINAL_MANAGER_H

#include <nlohmann/json.hpp>
#include <string>

namespace TerminalManager {

/**
 * 创建终端会话
 * @param session_id 会话ID
 * @return 操作结果的JSON对象
 */
nlohmann::json createTerminalSession(const std::string& session_id);

/**
 * 关闭终端会话
 * @param session_id 会话ID
 * @return 操作结果的JSON对象
 */
nlohmann::json closeTerminalSession(const std::string& session_id);

/**
 * 列出所有活动会话
 * @return 包含会话列表的JSON对象
 */
nlohmann::json listActiveSessions();

/**
 * 清理超时的会话
 */
void cleanupTimeoutSessions();

/**
 * 在指定会话中执行shell命令
 * @param command 要执行的命令
 * @param session_id 会话ID
 * @return 命令执行结果的JSON对象
 */
nlohmann::json executeShellCommand(const std::string& command,
                                   const std::string& session_id);

/**
 * 强制终止会话进程
 * @param session_id 会话ID
 * @return 操作结果的JSON对象
 */
nlohmann::json forceKillSession(const std::string& session_id);

/**
 * 设置输出长度限制
 * @param limit 最大输出长度（字节）
 */
void setOutputLengthLimit(size_t limit);

/**
 * 获取当前输出长度限制
 * @return 当前输出长度限制（字节）
 */
size_t getOutputLengthLimit();

/**
 * 清理输出，移除ANSI转义序列和多余的格式字符，并限制输出长度
 * @param rawOutput 原始输出字符串
 * @return 清理后的输出字符串
 */
std::string cleanOutput(const std::string& rawOutput);

/**
 * 获取当前工作目录
 * @return 当前工作目录的字符串
 */
std::string getCurrentWorkingDirectory();

/**
 * 从指定会话获取当前工作目录
 * @param session_id 会话ID
 * @return 会话的当前工作目录字符串
 */
std::string getCurrentWorkingDirectoryFromSession(
    const std::string& session_id);

}  // namespace TerminalManager

#endif  // TERMINAL_MANAGER_H