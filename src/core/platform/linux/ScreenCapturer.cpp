#include "core/ScreenCapturer.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "core/Logger.h"
#include "core/platform/linux/X11RAIIClasses.h"

std::optional<RawImage> ScreenCapturer::captureScreen() {
    X11DisplayRAII display(XOpenDisplay(nullptr));
    if (!display) {
        Logger::error("Cannot open X11 display");
        return std::nullopt;
    }

    Window root = DefaultRootWindow(display.get());
    XWindowAttributes attributes = {};
    if (!XGetWindowAttributes(display, root, &attributes)) {
        Logger::error("Failed to get window attributes");
        return std::nullopt;
    }

    XImageRAII img(XGetImage(display, root, 0, 0, attributes.width,
                             attributes.height, AllPlanes, ZPixmap));
    if (!img) {
        Logger::error("Failed to capture XImage");
        return std::nullopt;
    }

    RawImage result;
    result.width = attributes.width;
    result.height = attributes.height;
    result.pixels.resize(result.width * result.height * 3);

    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            unsigned long pixel = XGetPixel(img.get(), x, y);
            uint8_t blue = pixel & 0xff;
            uint8_t green = (pixel >> 8) & 0xff;
            uint8_t red = (pixel >> 16) & 0xff;

            int index = (y * result.width + x) * 3;
            result.pixels[index] = red;
            result.pixels[index + 1] = green;
            result.pixels[index + 2] = blue;
        }
    }

    return result;
}