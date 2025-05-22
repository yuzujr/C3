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
    int run();

private:
    // 初始化日志、读取配置、启用高 DPI 感知
    void init();
    // 启动命令轮询线程
    void startCommandPollingThread();
    // 主循环
    void mainLoop();
    // 上传图像和配置文件
    void uploadImageWithRetry(const std::vector<uint8_t>& frame,
                              const Config& config);
    void uploadConfigWithRetry(const Config& config);
    // 应用配置设置
    // 包括上传配置文件和设置开机自启
    void applyConfigSettings(const Config& config);

private:
    std::atomic<bool> m_running;
    Config m_config;
    ControlCenter m_controller;
    CommandDispatcher m_dispatcher;
    std::jthread m_commandPollingThread;
    static constexpr int kCommandPollingInterval = 3;  // 秒
};

#endif  // SCREEN_UPLOADER_APP_H
