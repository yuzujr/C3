#ifndef SCREEN_CAPTURER_H
#define SCREEN_CAPTURER_H

#include <opencv2/opencv.hpp>

class ScreenCapturer {
public:
    // 禁止创建实例
    ScreenCapturer() = delete;

    // 截取屏幕并返回 OpenCV Mat 对象
    static cv::Mat captureScreen();
};

#endif  // SCREEN_CAPTURER_H