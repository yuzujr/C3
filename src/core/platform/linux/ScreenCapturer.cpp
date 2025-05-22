#include "core/ScreenCapturer.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstring>
#include <vector>

#include "core/Logger.h"


RawImage ScreenCapturer::captureScreen() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        Logger::error("Cannot open X11 display");
        return {};
    }

    Window root = DefaultRootWindow(display);
    XWindowAttributes attributes = {};
    XGetWindowAttributes(display, root, &attributes);

    int width = attributes.width;
    int height = attributes.height;

    XImage* img =
        XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
    if (!img) {
        Logger::error("Failed to capture XImage");
        XCloseDisplay(display);
        return {};
    }

    // 输出 RGB 格式
    RawImage result;
    result.width = width;
    result.height = height;
    result.pixels.resize(width * height * 3);  // 3 channels: R, G, B

    // 解析 BGRA 或 BGRX 数据（通常为32位）
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned long pixel = XGetPixel(img, x, y);
            uint8_t blue = pixel & 0xff;
            uint8_t green = (pixel >> 8) & 0xff;
            uint8_t red = (pixel >> 16) & 0xff;

            int index = (y * width + x) * 3;
            result.pixels[index] = red;
            result.pixels[index + 1] = green;
            result.pixels[index + 2] = blue;
        }
    }

    XDestroyImage(img);
    XCloseDisplay(display);

    return result;
}
