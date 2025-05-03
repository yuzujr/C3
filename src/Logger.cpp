#include "Logger.h"

void Logger::log2stdout(const std::string& message) {
    std::cout << "[CLIENT] " << message << std::endl;
}

void Logger::log2stderr(const std::string& message) {
    std::cerr << "[CLIENT] " << message << std::endl;
}
