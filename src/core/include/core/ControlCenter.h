#ifndef CONTROLCENTER_H
#define CONTROLCENTER_H

#include <condition_variable>
#include <mutex>

/**
 * @brief ControlCenter 用于管理命令线程与主线程之间的同步状态。
 *        支持 pause / resume 控制。
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

    // 如果暂停，等待服务器 resume 命令
    void waitIfPaused();

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool paused_;
};

#endif  // CONTROLCENTER_H