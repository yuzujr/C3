#ifndef SCREEN_UPLOADER_H
#define SCREEN_UPLOADER_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

class ScreenUploader {
public:
    // 禁止创建实例
    ScreenUploader() = delete;
    ~ScreenUploader() = delete;

    // 截取屏幕并返回 OpenCV Mat 对象
    static cv::Mat captureScreenMat();

    // 使用 curl 发送内存中的图像数据（JPEG 编码）
    static bool uploadImage(const cv::Mat& frame, const std::string& url);

private:
    // 生成时间戳文件名
    static std::string generateTimestampFilename();
    
    // 将图像编码为 JPEG 格式
    static std::vector<uchar> encodeImageToJPEG(const cv::Mat& frame);
};

#endif  // SCREEN_UPLOADER_H