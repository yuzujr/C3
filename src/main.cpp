#include <signal.h>

#include <cstdlib>

#include "app/C3App.h"

// 全局应用实例指针，用于信号处理
C3App* g_appInstance = nullptr;

// 信号处理函数
void signalHandler(int signum) {
    Logger::info(
        std::format("Received signal {}, shutting down gracefully...", signum));

    // 如果应用实例存在，调用其统一的停止清理方法
    if (g_appInstance) {
        g_appInstance->stop();
    }

    exit(signum);
}

int main() {
    // 注册信号处理器
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    C3App app;
    g_appInstance = &app;  // 设置全局实例指针
    return app.run();
}