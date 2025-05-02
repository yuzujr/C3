#ifndef SCREEN_UPLOADER_H
#define SCREEN_UPLOADER_H

#include <curl/curl.h>

#include <chrono>
#include <ctime>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <vector>

#include "logger.h"

#ifdef _WIN32
#include <shellscalingapi.h>
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

class ScreenUploader {
public:
    ScreenUploader();
    ~ScreenUploader() = default;

    // 截取屏幕并返回 OpenCV Mat 对象
    cv::Mat captureScreenMat();

    // 使用 curl 发送内存中的图像数据（JPEG 编码）
    bool uploadImage(const cv::Mat& frame, const std::string& url);

private:
    // 生成时间戳文件名
    std::string generateTimestampFilename();
    // 将图像编码为 JPEG 格式
    std::vector<uchar> encodeImageToJPEG(const cv::Mat& frame);
};

#endif  // SCREEN_UPLOADER_H