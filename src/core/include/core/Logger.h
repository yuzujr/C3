#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string_view>

enum class LogTarget { Client, Server };

class Logger {
public:
    // 禁止创建实例
    Logger() = delete;

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

    // 输出warn级别日志，默认输出到client_logger
    static void warn(std::string_view msg,
                     LogTarget target = LogTarget::Client);

private:
    // Logger names
    static constexpr const char* CLIENT_LOGGER_NAME = "client";
    static constexpr const char* SERVER_LOGGER_NAME = "server";

    // 获取目标日志记录器
    static std::shared_ptr<spdlog::logger> get_logger(LogTarget target);
};

#endif  // LOGGER_H