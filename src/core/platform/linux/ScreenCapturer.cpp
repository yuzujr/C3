#include "core/ScreenCapturer.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <chrono>
#include <ctime>

#include "core/Logger.h"

// 屏幕截图为 cv::Mat
cv::Mat ScreenCapturer::captureScreen() {
    // X11 截图
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        Logger::log2stderr("Cannot open X11 display");
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
        Logger::log2stderr("Failed to capture XImage");
        XCloseDisplay(display);
        return {};
    }

    cv::Mat mat(height, width, CV_8UC4, img->data);
    mat = mat.clone();  // 拷贝数据，因为 img->data 是 X11 的内部数据

    XDestroyImage(img);
    XCloseDisplay(display);

    cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
    return mat;
}
