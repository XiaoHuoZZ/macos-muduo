/**
 * 基于Reacter的定时器
 */

#ifndef MACOS_MUDUO_NET_TIMER_QUEUE_H
#define MACOS_MUDUO_NET_TIMER_QUEUE_H

#ifdef __linux__

#include "muduo/base/utils.h"
#include "muduo/net/timer.h"
#include "muduo/net/channel.h"

#include <memory>
#include <set>
#include <vector>

namespace muduo::net {

class EventLoop;

class TimerQueue : noncopyable {
private:
    EventLoop* loop_;
    int timerfd_;
    std::set<std::shared_ptr<Timer>, TimerComparator> timers_;
    Channel timerfdChannel_;

public:

    TimerQueue(EventLoop* loop);

    ~TimerQueue();

    /**
    * 增加定时器，指定时间when执行TimerCallback
    */
    TimerId addTimer(SteadyTime when, double interval, Timer::TimerCallback cb);

    /**
    * 根据TimerId删除定时器
    */
    void cancel(const TimerId &id);

private:
    int createTimerfd();
    bool resetTimerfd(SteadyTime time);
    bool readTimerfd();

    void addTimerInLoop(std::shared_ptr<Timer> timer);
    void cancelInLoop(const TimerId &id);

    // 处理timerfd传来的读事件
    void handleRead();

    std::vector<std::shared_ptr<Timer>> getExpired(SteadyTime now);
};

} // namespace muduo::time

#endif //__linux__
#endif //MACOS_MUDUO_NET_TIMER_QUEUE_H
