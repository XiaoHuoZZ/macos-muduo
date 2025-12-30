#ifdef __linux__

#include "muduo/net/timer_queue.h"
#include "muduo/net/timer.h"
#include "muduo/net/eventloop.h"
#include "logger.h"
#include <memory>
#include <sys/timerfd.h>
#include <utility>

using muduo::net::TimerQueue;
using muduo::net::Timer;
using muduo::net::TimerId;

TimerQueue::TimerQueue(EventLoop* loop)
        : loop_(loop),
        timerfd_(createTimerfd()),
        timerfdChannel_(loop_, timerfd_) {
    
    // 生命周期Eventloop > TimerQueue，传this安全
    timerfdChannel_.setReadCallback([this](TimeStamp receive_time) {
        handleRead();
    });
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
}

int TimerQueue::createTimerfd()
{
    // 使用单调时钟，确保不会被用户修改系统时间影响
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                    TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        LOG_FATAL("create timerfd failed");
    }
    return timerfd;
}

bool TimerQueue::resetTimerfd(SteadyTime time)
{
    // 0初始化
    struct itimerspec new_value = {};
    new_value.it_value.tv_sec = time / 1000000000ULL;
    new_value.it_value.tv_nsec = time % 1000000000ULL;
        
    // 使用 TFD_TIMER_ABSTIME 绝对时间模式，防止漂移
    int ret = ::timerfd_settime(timerfd_, TFD_TIMER_ABSTIME, &new_value, NULL);
    if (ret < 0) {
        LOG_ERROR("timerfd_settime failed");
        return false;
    }
    return true;
}

bool TimerQueue::readTimerfd()
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd_, &howmany, sizeof howmany);
    if (n != sizeof howmany)
    {
        LOG_ERROR("timer fd reads {}", n);
        return false;
    }
    return true;
}

TimerId TimerQueue::addTimer(SteadyTime when, double interval, Timer::TimerCallback cb) {
    auto timer = std::make_shared<Timer>(when, interval, std::move(cb));
    TimerId id(timer);

    // 传this指针是安全的, TimerQueue属于EventLoop
    loop_->runInLoop([this, ptr = std::move(timer)]  () mutable {
        addTimerInLoop(std::move(ptr));
    });

    return id;
}

void TimerQueue::cancel(const TimerId &id)
{
    loop_->runInLoop([this, id]  ()  {
        cancelInLoop(id);
    });
}

void TimerQueue::addTimerInLoop(std::shared_ptr<Timer> timer) {
    loop_->assertInLoopThread();

    auto iter = timers_.begin();
    bool is_soonest = timers_.empty() ||
        timer->expiration() < (*iter)->expiration();
    auto expiration = timer->expiration();

    if (!timers_.insert(std::move(timer)).second) {
        LOG_FATAL("repeat timer insert");
        return;
    }
    
    // 如果该Timer为最紧急的，重新设置
    if (is_soonest) {
        resetTimerfd(expiration);
    }
}

void TimerQueue::cancelInLoop(const TimerId &id)
{
    loop_->assertInLoopThread();
    auto sptr = id.timer_.lock();

    // Timer存活, 有可能存在于timers_里面
    if (sptr != nullptr) {
        timers_.erase(sptr);
    }

    // 自注销情况(在回调里面cancel), 防止周期任务又加回去
    sptr->cancel();
}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();

    if (!readTimerfd()) {
        return;
    }

    auto expired = getExpired(time::getSteadyTime());
    // 执行用户回调
    for (const auto& time : expired)
    {
        time->execCallback();
    }

    for (auto& time : expired)
    {
        // 如果是周期性的，且未被取消，重新加入管理
        if (time->intervalMs() > 0 && !time->isCancel()) {
            time->restart();
            timers_.insert(std::move(time));
        }
    }

    // 设置下一轮定时器
    if (!timers_.empty()) {
        resetTimerfd((*timers_.begin())->expiration());
    }
}

std::vector<std::shared_ptr<Timer>> TimerQueue::getExpired(SteadyTime now)
{
    std::vector<std::shared_ptr<Timer>> expired;

    // 从左遍历到期定时器
    while (!timers_.empty() && (*timers_.begin())->expiration() <= now) {
        // C++17 extract
        // 这步之后，节点从树里摘除，树会进行一次 O(1) 级别的摊还旋转
        auto node = timers_.extract(timers_.begin());
        
        expired.push_back(std::move(node.value()));
    }

    return expired;
}

#endif //__linux__