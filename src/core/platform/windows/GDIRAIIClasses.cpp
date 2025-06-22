#include "core/platform/windows/GDIRAIIClasses.h"

#include <stdexcept>


// GDIContext 实现
GDIContext::GDIContext(HDC hdc) noexcept : m_hdc(hdc) {}

GDIContext::~GDIContext() {
    if (m_hdc) {
        DeleteDC(m_hdc);
    }
}

GDIContext::GDIContext(GDIContext&& other) noexcept : m_hdc(other.m_hdc) {
    other.m_hdc = nullptr;
}

GDIContext& GDIContext::operator=(GDIContext&& other) noexcept {
    if (this != &other) {
        reset();
        m_hdc = other.m_hdc;
        other.m_hdc = nullptr;
    }
    return *this;
}

GDIContext::operator bool() const noexcept {
    return m_hdc != nullptr;
}

GDIContext::operator HDC() const noexcept {
    return m_hdc;
}

void GDIContext::reset(HDC hdc) noexcept {
    if (m_hdc) {
        DeleteDC(m_hdc);
    }
    m_hdc = hdc;
}

HDC GDIContext::release() noexcept {
    HDC hdc = m_hdc;
    m_hdc = nullptr;
    return hdc;
}

HDC GDIContext::get() const noexcept {
    return m_hdc;
}

// GDIBitmap 实现
GDIBitmap::GDIBitmap(HBITMAP hbitmap) noexcept : m_hbitmap(hbitmap) {}

GDIBitmap::~GDIBitmap() {
    if (m_hbitmap) {
        DeleteObject(m_hbitmap);
    }
}

GDIBitmap::GDIBitmap(GDIBitmap&& other) noexcept : m_hbitmap(other.m_hbitmap) {
    other.m_hbitmap = nullptr;
}

GDIBitmap& GDIBitmap::operator=(GDIBitmap&& other) noexcept {
    if (this != &other) {
        reset();
        m_hbitmap = other.m_hbitmap;
        other.m_hbitmap = nullptr;
    }
    return *this;
}

GDIBitmap::operator bool() const noexcept {
    return m_hbitmap != nullptr;
}

GDIBitmap::operator HBITMAP() const noexcept {
    return m_hbitmap;
}

void GDIBitmap::reset(HBITMAP hbitmap) noexcept {
    if (m_hbitmap) {
        DeleteObject(m_hbitmap);
    }
    m_hbitmap = hbitmap;
}

HBITMAP GDIBitmap::release() noexcept {
    HBITMAP hbitmap = m_hbitmap;
    m_hbitmap = nullptr;
    return hbitmap;
}

HBITMAP GDIBitmap::get() const noexcept {
    return m_hbitmap;
}

// SelectObjectGuard 实现
SelectObjectGuard::SelectObjectGuard(HDC hdc, HGDIOBJ obj)
    : m_hdc(hdc), m_oldObj(SelectObject(hdc, obj)) {
    if (!m_oldObj) {
        throw std::runtime_error("SelectObject failed");
    }
}

SelectObjectGuard::~SelectObjectGuard() {
    if (m_hdc && m_oldObj) {
        SelectObject(m_hdc, m_oldObj);
    }
}