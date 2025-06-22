#ifndef X11_RAII_CLASSES_H
#define X11_RAII_CLASSES_H

#if defined(__linux__)

#include <X11/Xlib.h>

class X11DisplayRAII {
public:
    X11DisplayRAII(Display* display = nullptr) noexcept;
    ~X11DisplayRAII();

    // 禁止拷贝
    X11DisplayRAII(const X11DisplayRAII&) = delete;
    X11DisplayRAII& operator=(const X11DisplayRAII&) = delete;

    // 允许移动
    X11DisplayRAII(X11DisplayRAII&& other) noexcept;
    X11DisplayRAII& operator=(X11DisplayRAII&& other) noexcept;

    explicit operator bool() const noexcept;
    operator Display*() const noexcept;

    void reset(Display* display = nullptr) noexcept;
    Display* release() noexcept;
    Display* get() const noexcept;

private:
    Display* m_display;
};

class XImageRAII {
public:
    XImageRAII(XImage* image = nullptr) noexcept;
    ~XImageRAII();

    // 禁止拷贝
    XImageRAII(const XImageRAII&) = delete;
    XImageRAII& operator=(const XImageRAII&) = delete;

    // 允许移动
    XImageRAII(XImageRAII&& other) noexcept;
    XImageRAII& operator=(XImageRAII&& other) noexcept;

    explicit operator bool() const noexcept;
    operator XImage*() const noexcept;

    void reset(XImage* image = nullptr) noexcept;
    XImage* release() noexcept;
    XImage* get() const noexcept;

private:
    XImage* m_image;
};

#endif  // defined(__linux__)

#endif  // X11_RAII_CLASSES_H