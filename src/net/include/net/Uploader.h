#ifndef UPLOADER_H
#define UPLOADER_H

#include <cstdint>
#include <string>
#include <vector>

#include "core/Logger.h"
#include "nlohmann/json.hpp"

namespace cpr {
class Response;
}

class Uploader {
public:
    // 禁止创建实例
    Uploader() = delete;

    // 尝试上传图像
    static bool uploadImage(const std::vector<uint8_t>& frame,
                            const std::string& url);

    // 尝试上传图像（支持SSL配置）
    static bool uploadImageWithSSL(const std::vector<uint8_t>& frame,
                                   const std::string& url,
                                   bool skip_ssl_verification = false);

    // 尝试上传配置文件
    static bool uploadConfig(const nlohmann::json& config,
                             const std::string& url);

    // 尝试上传配置文件（支持SSL配置）
    static bool uploadConfigWithSSL(const nlohmann::json& config,
                                    const std::string& url,
                                    bool skip_ssl_verification = false);

    // 有重试机制的上传
    // 要求Func是一个可调用对象，函数签名为bool()
    template <typename Func>
        requires std::is_invocable_r_v<bool, Func>
    static bool uploadWithRetry(Func&& uploadFunc, int max_retries,
                                int retry_delay_ms) {
        if (uploadFunc()) {
            return true;
        }

        for (int attempt = 1; attempt <= max_retries; ++attempt) {
            auto start = std::chrono::steady_clock::now();
            Logger::info(
                std::format("retrying ({}/{})...", attempt, max_retries));

            if (uploadFunc()) return true;

            if (attempt < max_retries) {
                std::this_thread::sleep_until(
                    start + std::chrono::milliseconds(retry_delay_ms));
            }
        }
        return false;
    }

private:
    // 生成时间戳文件名
    static std::string generateTimestampFilename();

    // 处理上传响应
    static bool handleUploadResponse(const cpr::Response& r,
                                     const std::string& context = "Upload");
};

#endif  // UPLOADER_H