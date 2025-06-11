#include "core/ControlCenter.h"

ControlCenter::ControlCenter() : paused_(false) {}

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
