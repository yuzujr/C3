#ifndef SCREEN_CAPTURER_H
#define SCREEN_CAPTURER_H

#include <optional>

struct RawImage;

class ScreenCapturer {
public:
    // 禁止创建实例
    ScreenCapturer() = delete;

    // 截取屏幕并返回图像
    static std::optional<RawImage> captureScreen();
};

#endif  // SCREEN_CAPTURER_H
