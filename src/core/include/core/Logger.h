#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string_view>

enum class LogTarget { Client, Server };

class Logger {
public:
    // 初始化日志记录器，设置日志模式和日志级别
    static void init(
        spdlog::level::level_enum client_level = spdlog::level::info,
        spdlog::level::level_enum server_level = spdlog::level::info);

    // 输出info级别日志，默认输出到client_logger
    static void info(std::string_view msg,
                     LogTarget target = LogTarget::Client);

    // 输出error级别日志，默认输出到client_logger
    static void error(std::string_view msg,
                      LogTarget target = LogTarget::Client);

    // 输出debug级别日志，默认输出到client_logger
    static void debug(std::string_view msg,
                      LogTarget target = LogTarget::Client);

private:
    static inline std::shared_ptr<spdlog::logger> client_logger;
    static inline std::shared_ptr<spdlog::logger> server_logger;
};

#endif  // LOGGER_H