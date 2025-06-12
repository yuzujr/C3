#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <nlohmann/json.hpp>
#include <string>

namespace SystemUtils {

// 启用高 DPI 感知
void enableHighDPI();

// 添加到开机启动项
void addToStartup(const std::string& appName);

// 从开机启动项中移除
void removeFromStartup(const std::string& appName);

// 获取可执行文件路径
std::string getExecutablePath();

// 获取可执行文件所在目录
std::string getExecutableDir();

// 执行 Shell 命令
nlohmann::json executeShellCommand(const std::string& command,
                                   const std::string& session_id);

// 创建新的终端会话
nlohmann::json createTerminalSession(const std::string& session_id);

// 关闭终端会话
nlohmann::json closeTerminalSession(const std::string& session_id);

// 列出所有活动会话
nlohmann::json listActiveSessions();

// 清理超时会话
void cleanupTimeoutSessions();

// 设置输出长度限制 (字节)
void setOutputLengthLimit(size_t limit);

// 获取当前输出长度限制
size_t getOutputLengthLimit();

}  // namespace SystemUtils

#endif  // SYSTEMUTILS_H