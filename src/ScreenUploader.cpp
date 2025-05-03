#include "ScreenUploader.h"

ScreenUploader::ScreenUploader() {
#ifdef _WIN32
    // 启用高 DPI 感知
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif
}

// 屏幕截图为 cv::Mat
cv::Mat ScreenUploader::captureScreenMat() {
#ifdef _WIN32
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
#else
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
#endif
    return mat;
}

// 使用 curl 发送内存中的图像数据（JPEG 编码）
bool ScreenUploader::uploadImage(const cv::Mat& frame, const std::string& url) {
    if (frame.empty()) {
        Logger::log2stderr("frame is empty!");
        return false;
    }

    std::vector<uchar> imgData = encodeImageToJPEG(frame);
    if (imgData.empty()) {
        Logger::log2stderr("Failed to encode image to JPEG");
        return false;
    }

    std::string filename = generateTimestampFilename();

    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::log2stderr("curl init failed!");
        return false;
    }

    curl_mime* form = curl_mime_init(curl);
    curl_mimepart* field = curl_mime_addpart(form);

    curl_mime_name(field, "file");
    curl_mime_data(field, reinterpret_cast<const char*>(imgData.data()),
                   imgData.size());
    curl_mime_filename(field, filename.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.88.1");

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    bool success = false;
    if (res != CURLE_OK) {
        Logger::log2stderr("upload failed: " +
                           std::string(curl_easy_strerror(res)));
    } else if (http_code < 200 || http_code >= 300) {
        Logger::log2stderr("upload failed: HTTP response code " +
                           std::to_string(http_code));
    } else {
        Logger::log2stdout("File uploaded successfully: " + filename);
        success = true;
    }

    curl_mime_free(form);
    curl_easy_cleanup(curl);
    return success;
}

std::string ScreenUploader::generateTimestampFilename() {
    // 生成时间戳文件名
    std::ostringstream filename;
#ifdef _WIN32
    std::time_t t = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &t);
    filename << "screen_" << std::put_time(&localTime, "%Y%m%d_%H%M%S")
             << ".jpg";
#else
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&now_time);
    filename << "screen_" << std::put_time(localTime, "%Y%m%d_%H%M%S")
             << ".jpg";
#endif
    return filename.str();
}

std::vector<uchar> ScreenUploader::encodeImageToJPEG(const cv::Mat& frame) {
    std::vector<uchar> imgData;
    std::vector<int> compression_params = {cv::IMWRITE_JPEG_QUALITY,
                                           85};  // 85是画质参数
    if (cv::imencode(".jpg", frame, imgData, compression_params)) {
        return imgData;
    }
    return {};
}