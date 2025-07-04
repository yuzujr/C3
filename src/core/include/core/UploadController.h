#ifndef UPLOADCONTROLLER_H
#define UPLOADCONTROLLER_H

#include <condition_variable>
#include <mutex>

/**
 * @brief UploadController 用于控制主线程上传操作的暂停和恢复。
 */
class UploadController {
public:
    UploadController() = default;
    ~UploadController() = default;
    UploadController(const UploadController&) = delete;
    UploadController& operator=(const UploadController&) = delete;

    // 暂停上传
    void pause();

    // 恢复上传
    void resume();

    // 如果暂停，等待服务器 resume 命令
    void waitIfPaused();

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool paused_ = false;
};

#endif  // UPLOADCONTROLLER_H