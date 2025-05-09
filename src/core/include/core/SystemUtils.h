#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <string>

namespace SystemUtils {

// 添加到开机启动项
void addToStartup(const std::string& appName);

// 从开机启动项中移除
void removeFromStartup(const std::string& appName);

// 获取可执行文件路径
std::string getExecutablePath();

// 启用高 DPI 感知
void enableHighDPI();

}  // namespace SystemUtils

#endif  // SYSTEMUTILS_H