#include "core/Logger.h"

void Logger::init(spdlog::level::level_enum client_level,
                  spdlog::level::level_enum server_level) {
    // 设置日志格式
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%n] [%l] %v");

    // 创建控制台日志记录器 - spdlog registry 自动管理生命周期
    auto client_logger = spdlog::stdout_color_mt(CLIENT_LOGGER_NAME);
    client_logger->set_level(client_level);  // 设置控制台日志级别
    client_logger->flush_on(client_level);   // 设定刷新级别

    auto server_logger = spdlog::stdout_color_mt(SERVER_LOGGER_NAME);
    server_logger->set_level(server_level);  // 设置控制台日志级别
    server_logger->flush_on(server_level);   // 设定刷新级别
}

// 获取目标日志记录器
std::shared_ptr<spdlog::logger> Logger::get_logger(LogTarget target) {
    const char* logger_name =
        (target == LogTarget::Client) ? CLIENT_LOGGER_NAME : SERVER_LOGGER_NAME;
    return spdlog::get(logger_name);
}

void Logger::info(std::string_view msg, LogTarget target) {
    get_logger(target)->info(msg);
}

void Logger::error(std::string_view msg, LogTarget target) {
    get_logger(target)->error(msg);
}

void Logger::debug(std::string_view msg, LogTarget target) {
    get_logger(target)->debug(msg);
}

void Logger::warn(std::string_view msg, LogTarget target) {
    get_logger(target)->warn(msg);
}