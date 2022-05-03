
#include "muduo/net/eventloop.h"
#include "muduo/net/poller.h"
#include "muduo/net/channel.h"
#include <csignal>

using muduo::net::EventLoop;
using muduo::net::Channel;

//匿名namespace 让loop_in_this_thread只对当前文件有效, 防止别的文件通过extern拿到EventLoop对象
namespace {
    //记录本线程的唯一EventLoop对象
    thread_local EventLoop *loop_in_this_thread = nullptr;

    class IgnoreSigPipe
    {
    public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
            LOG_TRACE("ignore SIGPIPE");
        }
    };
    //设置全局静态变量，忽略SIGPIPE信号
    IgnoreSigPipe ignore;
}

//默认超时时间
const int kPollTimeMs = 20000;

EventLoop::EventLoop()
        : looping_(false),
          thread_id_(std::this_thread::get_id()),
          poller_(std::make_unique<Poller>(this)),
          quit_(false),
          calling_pending_functors_(false),
          wakeup_fds(createWakeupFd()),
          wakeup_channel_(std::make_unique<Channel>(this, wakeup_fds.first)) {

    LOG_TRACE("EventLoop created in thread {}", thread_id_);
    //如果该线程已经创建过EventLoop
    if (loop_in_this_thread) {
        LOG_FATAL("Another EventLoop exists in this thread");
        abort();
    } else {
        loop_in_this_thread = this;
    }

    //设置wakeup_channel_
    wakeup_channel_->setReadCallback([this](TimeStamp receive_time) { handleRead(); });
    wakeup_channel_->enableReading();
}

EventLoop::~EventLoop() {
    assert(!looping_);
    loop_in_this_thread = nullptr;
}

void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread(); //确保在loop线程中开启循环
    looping_ = true;
    quit_ = false;

    /**
     * 开启监听，对每次监听结果进行事件分发
     */
    while (!quit_) {
        activeChannels_.clear();
        TimeStamp poll_time = poller_->poll(kPollTimeMs, &activeChannels_);  //开启循环，得到的结果在activeChannels_里面
        for (auto &activeChannel: activeChannels_) {
            activeChannel->handleEvent(poll_time);    //事件分发（由Channel来做）
        }
        //执行pending中的方法
        doPendingFunctors();
    }

    LOG_TRACE("EventLoop stop looping");
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;

    //唤醒结束loop
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);    //防止不属于该EventLoop的Channel进来
    assertInLoopThread();                    //必须在IO线程来做
    poller_->updateChannel(channel);
}

std::pair<int, int> EventLoop::createWakeupFd() {
    int fd[2];
    if (pipe(fd) == -1) {
        LOG_ERROR("can't create pipe");
        abort();
    }
    return std::make_pair(fd[0], fd[1]);
}

void EventLoop::handleRead() const {
    char one = 1;
    ssize_t n = read(wakeup_fds.first, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::handleRead() reads {} bytes instead of 1", n);
    }
}

void EventLoop::wakeup() const {
    char one = 1;
    ssize_t n = write(wakeup_fds.second, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::wakeup() writes {} bytes instead of 1", n);
    }
}

void EventLoop::runInLoop(const EventLoop::Functor &cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor &cb) {
    /**
     * 互斥地将函数加入队列
     */
    {
        std::lock_guard lg(functors_mutex_);
        pending_functors_.push_back(cb);
    }

    /**
     * 唤醒
     * 1. 此时非IO线程，需要立即唤醒IO线程来执行pending_functor
     * 2. 若IO线程此时正在执行pending中的functor, 而该functor又调用了queueInLoop，那就得先
     * 提前向管道塞入一个字节，防止之后loop的时候新加入的functor不能及时执行
     */
    if (!isInLoopThread() || calling_pending_functors_) {
        wakeup();
    }
}

void EventLoop::doPendingFunctors() {
    //tmp用于暂存目前为止需要执行的函数，防止不断有functor加入到列表陷入死循环，正常的IO事件得不到相应
    std::vector<Functor> tmp;
    calling_pending_functors_ = true;

    {
        std::lock_guard lock(functors_mutex_);
        tmp.swap(pending_functors_);
    }

    for (auto &fun: tmp) {
        fun();
    }

    calling_pending_functors_ = false;
}

void EventLoop::removeChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->removeChannel(channel);
}



