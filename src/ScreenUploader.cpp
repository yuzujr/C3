#include "ScreenUploader.h"

#include <curl/curl.h>

#include <opencv2/opencv.hpp>
#include <vector>

#include "Logger.h"

// 使用 curl 发送内存中的图像数据（JPEG 编码）
bool ScreenUploader::uploadImage(const cv::Mat& frame, const std::string& url) {
    if (frame.empty()) {
        Logger::log2stderr("frame is empty!");
        return false;
    }

    std::vector<uchar> imgData = encodeImageToJPEG(frame);
    if (imgData.empty()) {
        Logger::log2stderr("Failed to encode image to JPEG");
        return false;
    }

    std::string filename = generateTimestampFilename();

    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::log2stderr("curl init failed!");
        return false;
    }

    curl_mime* form = curl_mime_init(curl);
    curl_mimepart* field = curl_mime_addpart(form);

    curl_mime_name(field, "file");
    curl_mime_data(field, reinterpret_cast<const char*>(imgData.data()),
                   imgData.size());
    curl_mime_filename(field, filename.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.88.1");

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    bool success = false;
    if (res != CURLE_OK) {
        Logger::log2stderr("upload failed: " +
                           std::string(curl_easy_strerror(res)));
    } else if (http_code < 200 || http_code >= 300) {
        Logger::log2stderr("upload failed: HTTP response code " +
                           std::to_string(http_code));
    } else {
        Logger::log2stdout("File uploaded successfully: " + filename);
        success = true;
    }

    curl_mime_free(form);
    curl_easy_cleanup(curl);
    return success;
}

std::vector<uchar> ScreenUploader::encodeImageToJPEG(const cv::Mat& frame) {
    std::vector<uchar> imgData;
    std::vector<int> compression_params = {cv::IMWRITE_JPEG_QUALITY,
                                           85};  // 85是画质参数
    if (cv::imencode(".jpg", frame, imgData, compression_params)) {
        return imgData;
    }
    return {};
}