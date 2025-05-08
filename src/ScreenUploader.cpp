#include "ScreenUploader.h"

#include <cpr/cpr.h>

#include <opencv2/opencv.hpp>
#include <vector>

#include "Logger.h"

// 使用 cpr 库上传图像
bool ScreenUploader::uploadImage(const cv::Mat& frame, const std::string& url) {
    if (frame.empty()) {
        Logger::log2stderr("frame is empty!");
        return false;
    }

    // 图像转换为 JPEG 格式
    std::vector<uchar> imgData = encodeImageToJPEG(frame);
    if (imgData.empty()) {
        Logger::log2stderr("Failed to encode image to JPEG");
        return false;
    }

    // 生成时间戳文件名
    std::string filename = generateTimestampFilename() + ".jpg";

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
        Logger::log2stderr("upload failed: " + r.error.message);
        return false;
    }

    if (r.status_code < 200 || r.status_code >= 300) {
        Logger::log2stderr("upload failed: HTTP " +
                           std::to_string(r.status_code));
        return false;
    }

    Logger::log2stdout("File uploaded successfully: " + filename);
    return true;
}

// 将图像转换为 JPEG 格式
std::vector<uchar> ScreenUploader::encodeImageToJPEG(const cv::Mat& frame) {
    std::vector<uchar> imgData;
    std::vector<int> compression_params = {cv::IMWRITE_JPEG_QUALITY,
                                           85};  // 85是画质参数
    if (cv::imencode(".jpg", frame, imgData, compression_params)) {
        return imgData;
    }
    return {};
}