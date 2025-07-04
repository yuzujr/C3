#include "core/UploadController.h"

void UploadController::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = true;
}

void UploadController::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (paused_) {
        paused_ = false;
        cv_.notify_all();
    }
}

void UploadController::waitIfPaused() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() {
        return !paused_;
    });
}