#include "core/Logger.h"

#include <iostream>

namespace Logger {

void log2stdout(const std::string& message) {
    std::cout << "[CLIENT] " << message << std::endl;
}

void log2stderr(const std::string& message) {
    std::cerr << "[CLIENT] " << message << std::endl;
}

}  // namespace Logger