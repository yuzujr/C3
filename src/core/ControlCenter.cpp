#include "core/ControlCenter.h"

ControlCenter::ControlCenter()
    : paused_(false), screenshot_requested_(false) {}

void ControlCenter::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = true;
}

void ControlCenter::resume() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        paused_ = false;
    }
    cv_.notify_all();
}

bool ControlCenter::isPaused() {
    std::lock_guard<std::mutex> lock(mutex_);
    return paused_;
}

void ControlCenter::waitIfPaused() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() { return !paused_; });
}

void ControlCenter::requestScreenshot() {
    screenshot_requested_.store(true);
}

bool ControlCenter::shouldScreenshot() {
    return screenshot_requested_.exchange(false);
}
