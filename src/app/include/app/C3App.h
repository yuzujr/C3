#ifndef C3_APP_H
#define C3_APP_H

#include <atomic>
#include <string>

#include "core/core.h"
#include "net/net.h"

class C3App {
public:
    C3App();
    ~C3App();

    // 启动应用程序
    int run();
    // 停止应用程序（用于信号处理）
    void stop();
    // 截图并上传（远程命令回调函数）
    void captureAndUpload();

private:
    // 启动 WebSocket 命令监听器
    void startWebSocketCommandListener();

    // 主循环
    void mainLoop();

    // 上传图像
    void uploadImageWithRetry(const std::vector<uint8_t>& frame);
    // 上传配置文件
    void uploadConfigWithRetry();

    // 应用配置设置
    // 包括上传配置文件和设置开机自启
    void applyConfigSettings();

private:
    std::atomic<bool> m_running;
    Config m_config;
    UploadController m_controller;
    CommandDispatcher m_dispatcher;
    WebSocketClient m_wsClient;

    std::string getConfigEndpoint() const {
        return std::format("client/client_config?client_id={}",
                           m_config.client_id);
    }
    std::string getScreenshotEndpoint() const {
        return std::format("client/screenshot?client_id={}",
                           m_config.client_id);
    }
    std::string getWebSocketEndpoint() const {
        return std::format("client/ws?type=client&client_id={}",
                           m_config.client_id);
    }
};

#endif  // C3_APP_H
