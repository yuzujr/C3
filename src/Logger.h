#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

class Logger {
public:
    // 客户端标准输出
    static void log2stdout(const std::string& message);

    // 客户端标准错误输出
    static void log2stderr(const std::string& message);
};

#endif  // LOGGER_H