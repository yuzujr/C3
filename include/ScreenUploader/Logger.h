#ifndef LOGGER_H
#define LOGGER_H

#include <string>

namespace Logger {

// 客户端标准输出
void log2stdout(const std::string& message);

// 客户端标准错误输出
void log2stderr(const std::string& message);

}  // namespace Logger

#endif  // LOGGER_H