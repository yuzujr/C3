#ifndef SCREEN_CAPTURER_H
#define SCREEN_CAPTURER_H

#include <cstdint>
#include <vector>

#include "core/RawImage.h"

class ScreenCapturer {
public:
    // 禁止创建实例
    ScreenCapturer() = delete;

    // 截取屏幕并返回图像
    static RawImage captureScreen();
};

#endif  // SCREEN_CAPTURER_H
