
#include "ScreenUploader.h"

#include <windows.h>

#include <ctime>
#include <sstream>

// 屏幕截图为 cv::Mat
cv::Mat ScreenUploader::captureScreenMat() {
    // Windows GDI截图
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap =
        CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
    HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0,
           SRCCOPY);
    hBitmap = (HBITMAP)SelectObject(hMemoryDC, oldBitmap);

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    cv::Mat mat(bmp.bmHeight, bmp.bmWidth, CV_8UC4);
    GetBitmapBits(hBitmap, bmp.bmHeight * bmp.bmWidthBytes, mat.data);

    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);

    return mat;
}

std::string ScreenUploader::generateTimestampFilename() {
    // 生成时间戳文件名
    std::ostringstream filename;
    std::time_t t = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &t);
    filename << "screen_" << std::put_time(&localTime, "%Y%m%d_%H%M%S")
             << ".jpg";
    return filename.str();
}
