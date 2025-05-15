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

    ControlCenter(const ControlCenter&);

    // 暂停 / 继续上传控制
    void pause();
    void resume();

    // 外部等待 resume（主线程调用）
    void waitIfPaused();

    // 屏幕截图控制
    void requestScreenshot();
    bool consumeScreenshotRequest();  // 检查并消费一次请求

    // 只要收到截图请求就提前醒的sleep_until
    void interruptibleSleepUntil(
        const std::chrono::steady_clock::time_point& time_point);

    void setNextUploadTime(const std::chrono::steady_clock::time_point& time_point);

    std::chrono::steady_clock::time_point nextUploadTime();

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool paused_;
    //上次暂停时间点
    std::chrono::steady_clock::time_point pause_time_;
    //下次上传时间点
    std::chrono::steady_clock::time_point next_upload_time_;
    std::atomic<bool> screenshot_requested_;
};

#endif  // CONTROLCENTER_H