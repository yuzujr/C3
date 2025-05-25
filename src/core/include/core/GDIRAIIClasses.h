#ifndef GDI_RAII_CLASSES_H
#define GDI_RAII_CLASSES_H

#ifdef _WIN32

#include <windows.h>

// GDI设备上下文RAII包装
class GDIContext {
public:
    explicit GDIContext(HDC hdc = nullptr) noexcept;
    ~GDIContext();
    
    // 禁止拷贝
    GDIContext(const GDIContext&) = delete;
    GDIContext& operator=(const GDIContext&) = delete;
    
    // 允许移动
    GDIContext(GDIContext&& other) noexcept;
    GDIContext& operator=(GDIContext&& other) noexcept;
    
    // 转换操作符
    explicit operator bool() const noexcept;
    operator HDC() const noexcept;
    
    // 资源管理
    void reset(HDC hdc = nullptr) noexcept;
    HDC release() noexcept;
    HDC get() const noexcept;

private:
    HDC m_hdc;
};

// GDI位图RAII包装
class GDIBitmap {
public:
    explicit GDIBitmap(HBITMAP hbitmap = nullptr) noexcept;
    ~GDIBitmap();
    
    // 禁止拷贝
    GDIBitmap(const GDIBitmap&) = delete;
    GDIBitmap& operator=(const GDIBitmap&) = delete;
    
    // 允许移动
    GDIBitmap(GDIBitmap&& other) noexcept;
    GDIBitmap& operator=(GDIBitmap&& other) noexcept;
    
    // 转换操作符
    explicit operator bool() const noexcept;
    operator HBITMAP() const noexcept;
    
    // 资源管理
    void reset(HBITMAP hbitmap = nullptr) noexcept;
    HBITMAP release() noexcept;
    HBITMAP get() const noexcept;

private:
    HBITMAP m_hbitmap;
};

// GDI选对象保护器
class SelectObjectGuard {
public:
    SelectObjectGuard(HDC hdc, HGDIOBJ obj);
    ~SelectObjectGuard();
    
    // 禁止拷贝和移动
    SelectObjectGuard(const SelectObjectGuard&) = delete;
    SelectObjectGuard& operator=(const SelectObjectGuard&) = delete;
    SelectObjectGuard(SelectObjectGuard&&) = delete;
    SelectObjectGuard& operator=(SelectObjectGuard&&) = delete;

private:
    HDC m_hdc;
    HGDIOBJ m_oldObj;
};

#endif // _WIN32

#endif // GDI_RAII_CLASSES_H