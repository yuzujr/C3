#include "net/Uploader.h"

#include <cpr/cpr.h>

#include <format>
#include <opencv2/opencv.hpp>
#include <vector>

#include "core/Logger.h"
#include "net/Uploader.h"

bool Uploader::uploadImage(const cv::Mat& frame, const std::string& url) {
    if (frame.empty()) {
        Logger::error("frame is empty!");
        return false;
    }

    // 图像转换为 JPEG 格式
    std::vector<uchar> imgData = encodeImageToJPEG(frame);
    if (imgData.empty()) {
        Logger::error("Failed to encode image to JPEG");
        return false;
    }

    // 生成时间戳文件名
    std::string filename = std::format("{}.jpg", generateTimestampFilename());

    // 构造 POST 请求
    cpr::Buffer buffer{
        imgData.begin(),  // 首地址
        imgData.end(),    // 尾地址
        filename          // 文件名
    };

    cpr::Multipart form = {cpr::Part{"file", buffer}};

    cpr::Response r =
        cpr::Post(cpr::Url{url}, form,
                  cpr::Header{{"User-Agent", "cpr/1.11.0"}, {"Accept", "*/*"}});

    if (r.error) {
        Logger::error(std::format("Upload failed: {}", r.error.message));
        return false;
    }

    if (r.status_code < 200 || r.status_code >= 300) {
        std::string errorMsg =
            std::format("Upload failed: HTTP {}", r.status_code);
        if (!r.text.empty()) {
            errorMsg += std::format(" - Message: {}", r.text);
        }
        Logger::error(errorMsg);
        return false;
    }

    if (!r.text.empty()) {
        Logger::info(r.text, LogTarget::Server);
    }

    Logger::info(std::format("File uploaded successfully: {}\n", filename));
    return true;
}

bool Uploader::uploadWithRetry(const cv::Mat& image, const std::string& url,
                               int max_retries, int retry_delay_ms) {
    if (uploadImage(image, url)) {
        return true;
    }

    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        auto start = std::chrono::high_resolution_clock::now();
        Logger::info(std::format("retrying ({}/{})...", attempt, max_retries));

        if (uploadImage(image, url)) return true;

        if (attempt < max_retries) {
            std::this_thread::sleep_until(
                start + std::chrono::milliseconds(retry_delay_ms));
        }
    }

    return false;
}

std::vector<uchar> Uploader::encodeImageToJPEG(const cv::Mat& frame) {
    std::vector<uchar> imgData;
    std::vector<int> compression_params = {cv::IMWRITE_JPEG_QUALITY,
                                           85};  // 85是画质参数
    if (cv::imencode(".jpg", frame, imgData, compression_params)) {
        return imgData;
    }
    return {};
}

std::string Uploader::generateTimestampFilename() {
    std::ostringstream filename;
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&now_time);
    filename << "screen_" << std::put_time(localTime, "%Y%m%d_%H%M%S");
    return filename.str();
}