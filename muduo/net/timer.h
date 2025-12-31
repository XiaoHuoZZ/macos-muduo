/**
 * Timer
 */

#ifndef MACOS_MUDUO_NET_TIMER_H
#define MACOS_MUDUO_NET_TIMER_H

#include "utils.h"
#include <atomic>

namespace muduo::net {

class Timer {
public:
    using TimerCallback = std::function<void()>;
private:
    // 使用单调时间，方便后续使用TFD_TIMER_ABSTIME
    SteadyTime expiration_;
    // ms
    uint64_t intervalMs_;
    TimerCallback cb_;
    // 标记是否已经被删除，防止周期任务又加回去
    bool isCancel_;

public:
    Timer(SteadyTime when, uint64_t intervalMs, TimerCallback cb)
        : expiration_(when), intervalMs_(intervalMs), cb_(std::move(cb)),
          isCancel_(false) {}

    ~Timer() = default;

    SteadyTime expiration() const { return expiration_; };
    uint64_t intervalMs() const { return intervalMs_; };
    bool isCancel() const { return isCancel_; };

    void execCallback() { cb_(); };

    void restart() { expiration_ += intervalMs_ * 1000000; };
    void cancel() { isCancel_ = true; };
};

class TimerId {
private:
    std::weak_ptr<Timer> timer_;
    uint64_t id_;
    static inline std::atomic_int seq_ = 0;
public:
    friend class TimerQueue;
    TimerId(const std::shared_ptr<Timer> &timer) : timer_(timer), id_(seq_++) {};

    bool operator==(const TimerId& other) const {
        return id_ == other.id_;
    }

    bool operator!=(const TimerId& other) const {
        return id_ != other.id_;
    }
};


struct TimerComparator {
    bool operator()(const std::shared_ptr<Timer>& lhs, 
                    const std::shared_ptr<Timer>& rhs) const {
        if (!lhs || !rhs) return lhs < rhs;

        // 核心逻辑：比较到期时间
        if (lhs->expiration() != rhs->expiration()) {
            return lhs->expiration() < rhs->expiration();
        }

        // 如果两个定时器时间完全一样，必须通过指针地址区分。
        // 否则 set 会认为这是同一个对象，导致其中一个无法插入！
        return lhs.get() < rhs.get();
    }
};

} // namespace muduo::time
#endif //MACOS_MUDUO_NET_TIMER_H
