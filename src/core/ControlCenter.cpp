#include "core/ControlCenter.h"


ControlCenter::ControlCenter() : paused_(false), screenshot_requested_(false) {}

void ControlCenter::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = true;
    pause_time_ = std::chrono::steady_clock::now();
}

void ControlCenter::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (paused_) {
        auto now = std::chrono::steady_clock::now();
        auto paused_duration = now - pause_time_;
        next_upload_time_ += paused_duration;  // 延后上传时间点
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

void ControlCenter::setNextUploadTime(
    const std::chrono::steady_clock::time_point& time_point) {
        next_upload_time_ = time_point;
    }

    std::chrono::steady_clock::time_point ControlCenter::nextUploadTime() {
        return next_upload_time_;
    }
