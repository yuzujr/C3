#include "core/Logger.h"

void Logger::init(spdlog::level::level_enum client_level,
                  spdlog::level::level_enum server_level) {
    // 设置日志格式
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%n] [%l] %v");

    // 创建控制台日志记录器
    client_logger = spdlog::stdout_color_mt("client");
    client_logger->set_level(client_level);  // 设置控制台日志级别
    client_logger->flush_on(client_level);   // 设定刷新级别

    server_logger = spdlog::stdout_color_mt("server");
    server_logger->set_level(server_level);  // 设置控制台日志级别
    server_logger->flush_on(server_level);   // 设定刷新级别
}

void Logger::info(std::string_view msg, LogTarget target) {
    if (target == LogTarget::Client) {
        client_logger->info(msg);
    }
    if (target == LogTarget::Server) {
        server_logger->info(msg);
    }
}

void Logger::error(std::string_view msg, LogTarget target) {
    if (target == LogTarget::Client) {
        client_logger->error(msg);
    }
    if (target == LogTarget::Server) {
        server_logger->error(msg);
    }
}

void Logger::debug(std::string_view msg, LogTarget target) {
    if (target == LogTarget::Client) {
        client_logger->debug(msg);
    }
    if (target == LogTarget::Server) {
        server_logger->debug(msg);
    }
}

void Logger::warn(std::string_view msg, LogTarget target) {
    if (target == LogTarget::Client) {
        client_logger->warn(msg);
    }
    if (target == LogTarget::Server) {
        server_logger->warn(msg);
    }
}