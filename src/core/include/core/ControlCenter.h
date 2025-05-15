#ifndef CONTROLCENTER_H
#define CONTROLCENTER_H

#include <mutex>
#include <condition_variable>
#include <atomic>

/**
 * @brief ControlCenter 用于管理命令线程与主线程之间的同步状态。
 *        支持 pause / resume 控制、屏幕截图请求等功能。
 */
class ControlCenter {
public:
    ControlCenter();

    // 暂停 / 继续上传控制
    void pause();
    void resume();
    bool isPaused();

    // 外部等待 resume（主线程调用）
    void waitIfPaused();

    // 屏幕截图控制
    void requestScreenshot();
    bool shouldScreenshot(); // 检查并消费一次请求

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool paused_;
    std::atomic<bool> screenshot_requested_;
};

#endif // CONTROLCENTER_H
