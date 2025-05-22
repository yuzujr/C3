#include "core/ScreenCapturer.h"

#include <windows.h>

#include "core/Logger.h"

RawImage ScreenCapturer::captureScreen() {
    HDC hScreenDC = GetDC(nullptr);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap =
        CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
    HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);

    if (!BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0,
                SRCCOPY)) {
        Logger::error("BitBlt failed");
        SelectObject(hMemoryDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(nullptr, hScreenDC);
        return {};
    }

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    int bytesPerPixel = 4;  // Usually 32-bit BGRA
    int imageSize = bmp.bmWidth * bmp.bmHeight * bytesPerPixel;

    std::vector<uint8_t> buffer(imageSize);
    GetBitmapBits(hBitmap, imageSize, buffer.data());

    RawImage result;
    result.width = bmp.bmWidth;
    result.height = bmp.bmHeight;
    result.pixels.resize(result.width * result.height * 3);  // RGB output

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

    SelectObject(hMemoryDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);

    return result;
}
