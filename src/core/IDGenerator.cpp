#include "core/IDGenerator.h"

#include <chrono>
#include <random>
#include <sstream>

namespace IDGenerator {

std::string generateUUID() {
    // 使用随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);  // 生成十六进制数

    std::stringstream ss;
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now)
                       .count();  // 时间戳

    // 生成类似 UUID 格式的 ID
    ss << std::hex << seconds << "-";
    for (int i = 0; i < 4; ++i) {
        ss << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 4; ++i) {
        ss << dis(gen);
    }
    return ss.str();
}

}  // namespace IDGenerator