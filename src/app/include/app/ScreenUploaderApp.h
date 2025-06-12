#ifndef SCREEN_UPLOADER_APP_H
#define SCREEN_UPLOADER_APP_H

#include <atomic>
#include <thread>

#include "core/core.h"
#include "net/net.h"

class ScreenUploaderApp {
public:
    ScreenUploaderApp();
    ~ScreenUploaderApp();
    // 启动应用程序
    int run();                 // 立即截图并上传（远程命令回调函数）
    void takeScreenshotNow();  // 执行 Shell 命令（远程命令回调函数）

private:
    // 连接命令接收websocket
    void startWebSocketCommandListener();
    // 主循环
    void mainLoop();
    // 执行一次截图和上传
    void performScreenshotUpload();  // 上传图像和配置文件
    void uploadImageWithRetry(const std::vector<uint8_t>& frame,
                              const Config& config);
    void uploadConfigWithRetry(const Config& config);

    // 通用上传方法
    template <typename UploadFunc>
    void performUploadWithRetry(const std::string& endpoint,
                                const std::string& description,
                                UploadFunc uploadFunc, const Config& config);
    // 应用配置设置
    // 包括上传配置文件和设置开机自启
    void applyConfigSettings(const Config& config);

private:
    std::atomic<bool> m_running;
    Config m_config;
    ControlCenter m_controller;
    CommandDispatcher m_dispatcher;
    WebSocketClient m_wsClient;
};

#endif  // SCREEN_UPLOADER_APP_H
