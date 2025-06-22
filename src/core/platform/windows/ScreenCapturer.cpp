#include "core/ScreenCapturer.h"

#include <windows.h>

#include "core/Logger.h"
#include "core/platform/windows/GDIRAIIClasses.h"

std::optional<RawImage> ScreenCapturer::captureScreen() {
    // 获取屏幕DC
    HDC hScreenDC = GetDC(nullptr);
    if (!hScreenDC) {
        Logger::error("GetDC failed");
        return std::nullopt;
    }
    auto screenDcGuard = std::shared_ptr<void>(nullptr, [hScreenDC](void*) {
        ReleaseDC(nullptr, hScreenDC);
    });

    // 创建内存DC
    GDIContext memoryDC(CreateCompatibleDC(hScreenDC));
    if (!memoryDC) {
        Logger::error("CreateCompatibleDC failed");
        return std::nullopt;
    }

    // 创建兼容位图 (RAII自动管理DeleteObject)
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    GDIBitmap bitmap(
        CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight));
    if (!bitmap) {
        Logger::error("CreateCompatibleBitmap failed");
        return std::nullopt;
    }

    // 选择位图到DC (RAII自动恢复原对象)
    SelectObjectGuard selectGuard(memoryDC, bitmap);

    // 执行位块传输
    if (!BitBlt(memoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0,
                SRCCOPY)) {
        Logger::error("BitBlt failed");
        return std::nullopt;
    }

    // 获取位图信息
    BITMAP bmp;
    if (!GetObject(bitmap, sizeof(BITMAP), &bmp)) {
        Logger::error("GetObject failed");
        return std::nullopt;
    }

    // 读取位图数据
    int bytesPerPixel = 4;  // 通常为32位BGRA
    int imageSize = bmp.bmWidth * bmp.bmHeight * bytesPerPixel;
    std::vector<uint8_t> buffer(imageSize);
    if (!GetBitmapBits(bitmap, imageSize, buffer.data())) {
        Logger::error("GetBitmapBits failed");
        return std::nullopt;
    }

    // 转换到RGB格式
    RawImage result;
    result.width = bmp.bmWidth;
    result.height = bmp.bmHeight;
    result.pixels.resize(result.width * result.height * 3);  // RGB输出

    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            int srcIndex = (y * result.width + x) * bytesPerPixel;
            int dstIndex = (y * result.width + x) * 3;

            uint8_t blue = buffer[srcIndex + 0];
            uint8_t green = buffer[srcIndex + 1];
            uint8_t red = buffer[srcIndex + 2];

            result.pixels[dstIndex + 0] = red;
            result.pixels[dstIndex + 1] = green;
            result.pixels[dstIndex + 2] = blue;
        }
    }

    return result;
}
