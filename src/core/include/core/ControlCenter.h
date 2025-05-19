#ifndef CONTROLCENTER_H
#define CONTROLCENTER_H

#include <atomic>
#include <condition_variable>
#include <mutex>

/**
 * @brief ControlCenter 用于管理命令线程与主线程之间的同步状态。
 *        支持 pause / resume 控制、屏幕截图请求等功能。
 */
class ControlCenter {
public:
    ControlCenter();
    ~ControlCenter() = default;
    ControlCenter(const ControlCenter&) = delete;
    ControlCenter& operator=(const ControlCenter&) = delete;

    // 暂停上传
    void pause();

    // 恢复上传
    void resume();

    // 暂停，等待服务器 resume 命令
    void waitIfPaused();

    // 请求截图
    void requestScreenshot();

    // 检查并消费一次截图请求
    bool consumeScreenshotRequest();

    // 只要收到截图请求就提前醒的sleep_until
    void interruptibleSleepUntil(
        const std::chrono::steady_clock::time_point& time_point);

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool paused_;
    std::atomic<bool> screenshot_requested_;
};

#endif  // CONTROLCENTER_H