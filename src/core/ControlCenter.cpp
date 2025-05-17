#include "core/ControlCenter.h"

ControlCenter::ControlCenter() : paused_(false), screenshot_requested_(false) {}

void ControlCenter::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = true;
}

void ControlCenter::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (paused_) {
        paused_ = false;
        cv_.notify_all();
    }
}

void ControlCenter::waitIfPaused() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() {
        return !paused_;
    });
}

void ControlCenter::requestScreenshot() {
    screenshot_requested_.store(true);
    cv_.notify_all();
}

bool ControlCenter::consumeScreenshotRequest() {
    return screenshot_requested_.exchange(false);
}

void ControlCenter::interruptibleSleepUntil(
    const std::chrono::steady_clock::time_point& time_point) {
    std::unique_lock lock(mutex_);
    cv_.wait_until(lock, time_point, [&] {
        return screenshot_requested_.load();  // 只要收到截图请求就提前醒
    });
}
