#include "core/ScreenCapturer.h"

#include <windows.h>

#include "core/Logger.h"
#include "core/RawImage.h"
#include "core/platform/windows/GDIRAIIClasses.h"

struct CaptureContext {
    std::vector<RawImage>* out;
};

static BOOL monitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM lParam);

std::vector<RawImage> ScreenCapturer::captureAllMonitors() {
    std::vector<RawImage> results;

    CaptureContext ctx{&results};

    if (!EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc,
                             reinterpret_cast<LPARAM>(&ctx))) {
        Logger::error("EnumDisplayMonitors failed");
    }

    return results;
}

static BOOL monitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM lParam) {
    auto* ctx = reinterpret_cast<CaptureContext*>(lParam);
    MONITORINFOEXA mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoA(hMon, &mi)) {
        Logger::warn("Failed to get monitor info");
        return TRUE;
    }

    RECT r = mi.rcMonitor;
    int width = r.right - r.left;
    int height = r.bottom - r.top;

    // 获取屏幕DC
    HDC hScreenDC = GetDC(nullptr);
    if (!hScreenDC) {
        Logger::warn("GetDC failed");
        return TRUE;
    }
    auto screenDcGuard = std::shared_ptr<void>(nullptr, [hScreenDC](void*) {
        ReleaseDC(nullptr, hScreenDC);
    });

    // 创建兼容内存DC
    GDIContext memoryDC(CreateCompatibleDC(hScreenDC));
    if (!memoryDC) {
        Logger::warn("CreateCompatibleDC failed");
        return TRUE;
    }

    // 创建位图
    GDIBitmap bitmap(CreateCompatibleBitmap(hScreenDC, width, height));
    if (!bitmap) {
        Logger::warn("CreateCompatibleBitmap failed");
        return TRUE;
    }

    // 选择到 DC
    SelectObjectGuard selectGuard(memoryDC, bitmap);

    // 截图
    if (!BitBlt(memoryDC, 0, 0, width, height, hScreenDC, r.left, r.top,
                SRCCOPY)) {
        Logger::warn(std::format("BitBlt failed for monitor: {}", mi.szDevice));
        return TRUE;
    }

    // 获取位图信息
    BITMAP bmp;
    if (!GetObject(bitmap, sizeof(BITMAP), &bmp)) {
        Logger::warn("GetObject failed");
        return TRUE;
    }

    // 获取像素数据（BGRA）
    const int bytesPerPixel = 4;
    int imageSize = bmp.bmWidth * bmp.bmHeight * bytesPerPixel;
    std::vector<uint8_t> buffer(imageSize);
    if (!GetBitmapBits(bitmap, imageSize, buffer.data())) {
        Logger::warn("GetBitmapBits failed");
        return TRUE;
    }

    // 转换为RGB格式
    RawImage result;
    result.width = bmp.bmWidth;
    result.height = bmp.bmHeight;
    result.pixels.resize(result.width * result.height * 3);

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

    ctx->out->push_back(std::move(result));
    return TRUE;
}