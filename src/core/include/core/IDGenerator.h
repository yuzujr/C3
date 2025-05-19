#ifndef IDGENERATOR_H
#define IDGENERATOR_H
#include <string>

class IDGenerator {
public:
    // 禁止创建实例
    IDGenerator() = delete;

    // 生成一个唯一的UUID
    static std::string generateUUID();
};

#endif  // IDGENERATOR_H