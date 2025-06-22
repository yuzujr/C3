#include "core/platform/linux/X11RAIIClasses.h"

#include <X11/Xutil.h>

// X11DisplayRAII 实现
X11DisplayRAII::X11DisplayRAII(Display* display) noexcept
    : m_display(display) {}

X11DisplayRAII::~X11DisplayRAII() {
    if (m_display) {
        XCloseDisplay(m_display);
    }
}

X11DisplayRAII::X11DisplayRAII(X11DisplayRAII&& other) noexcept
    : m_display(other.m_display) {
    other.m_display = nullptr;
}

X11DisplayRAII& X11DisplayRAII::operator=(X11DisplayRAII&& other) noexcept {
    if (this != &other) {
        reset();
        m_display = other.m_display;
        other.m_display = nullptr;
    }
    return *this;
}

X11DisplayRAII::operator bool() const noexcept {
    return m_display != nullptr;
}
X11DisplayRAII::operator Display*() const noexcept {
    return m_display;
}

void X11DisplayRAII::reset(Display* display) noexcept {
    if (m_display) {
        XCloseDisplay(m_display);
    }
    m_display = display;
}

Display* X11DisplayRAII::release() noexcept {
    Display* display = m_display;
    m_display = nullptr;
    return display;
}

Display* X11DisplayRAII::get() const noexcept {
    return m_display;
}

// XImageRAII 实现
XImageRAII::XImageRAII(XImage* image) noexcept : m_image(image) {}

XImageRAII::~XImageRAII() {
    if (m_image) {
        XDestroyImage(m_image);
    }
}

XImageRAII::XImageRAII(XImageRAII&& other) noexcept : m_image(other.m_image) {
    other.m_image = nullptr;
}

XImageRAII& XImageRAII::operator=(XImageRAII&& other) noexcept {
    if (this != &other) {
        reset();
        m_image = other.m_image;
        other.m_image = nullptr;
    }
    return *this;
}

XImageRAII::operator bool() const noexcept {
    return m_image != nullptr;
}
XImageRAII::operator XImage*() const noexcept {
    return m_image;
}

void XImageRAII::reset(XImage* image) noexcept {
    if (m_image) {
        XDestroyImage(m_image);
    }
    m_image = image;
}

XImage* XImageRAII::release() noexcept {
    XImage* image = m_image;
    m_image = nullptr;
    return image;
}

XImage* XImageRAII::get() const noexcept {
    return m_image;
}