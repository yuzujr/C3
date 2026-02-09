#include "core/ScreenCapturer.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

#include "core/Logger.h"
#include "core/RawImage.h"
#include "core/platform/linux/X11RAIIClasses.h"

#ifdef C3_HAS_WLR_SCREENCOPY
#include "core/platform/linux/WlrScreenCapturer.h"
#endif

std::vector<RawImage> ScreenCapturer::captureAllMonitors() {
#ifdef C3_HAS_WLR_SCREENCOPY
    // 优先尝试 Wayland wlr-screencopy 协议
    if (WlrScreenCapturer::isAvailable()) {
        Logger::info("Using wlr-screencopy (Wayland) backend");
        auto images = WlrScreenCapturer::captureAllMonitors();
        if (!images.empty()) {
            return images;
        }
        Logger::warn("wlr-screencopy capture failed, falling back to X11");
    }
#endif

    std::vector<RawImage> images;

    X11DisplayRAII display(XOpenDisplay(nullptr));
    if (!display) {
        Logger::error("Cannot open X11 display");
        return images;
    }

    Window root = DefaultRootWindow(display.get());

    // 检查是否启用 Xinerama
    if (!XineramaIsActive(display.get())) {
        Logger::warn(
            "Xinerama not active, capturing full root window as fallback.");

        XWindowAttributes attributes = {};
        if (!XGetWindowAttributes(display.get(), root, &attributes)) {
            Logger::error("Failed to get window attributes");
            return images;
        }

        XImageRAII img(XGetImage(display.get(), root, 0, 0, attributes.width,
                                 attributes.height, AllPlanes, ZPixmap));
        if (!img) {
            Logger::error("Failed to capture root XImage");
            return images;
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

        images.push_back(std::move(result));
        return images;
    }

    int screenCount = 0;
    XineramaScreenInfo* screens =
        XineramaQueryScreens(display.get(), &screenCount);
    if (!screens) {
        Logger::error("XineramaQueryScreens failed");
        return images;
    }

    for (int i = 0; i < screenCount; ++i) {
        const auto& screen = screens[i];
        Logger::info(std::format("Capturing screen {}: {}x{} @ ({}, {})", i,
                                 screen.width, screen.height, screen.x_org,
                                 screen.y_org));

        XImageRAII img(XGetImage(display.get(), root, screen.x_org,
                                 screen.y_org, screen.width, screen.height,
                                 AllPlanes, ZPixmap));
        if (!img) {
            Logger::warn(
                std::format("Failed to capture image for screen {}", i));
            continue;
        }

        RawImage result;
        result.width = screen.width;
        result.height = screen.height;
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

        images.push_back(std::move(result));
    }

    XFree(screens);
    return images;
}