
#include "muduo/net/event_loop.h"
#include "muduo/net/poller.h"
#include "muduo/net/channel.h"

using muduo::net::EventLoop;
using muduo::net::Channel;

//匿名namespace 让loop_in_this_thread只对当前文件有效, 防止别的文件通过extern拿个EventLoop对象
namespace {
    //记录本线程的唯一EventLoop对象
    thread_local EventLoop *loop_in_this_thread = nullptr;
}

//默认超时时间
const int kPollTimeMs = 10000;

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
    wakeup_channel_->setReadCallback([this] { handleRead(); });
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
        poller_->poll(kPollTimeMs, &activeChannels_);  //开启循环，得到的结果在activeChannels_里面
        for (auto & activeChannel : activeChannels_) {
            activeChannel->handleEvent();    //事件分发（由Channel来做）
        }
    }

    LOG_TRACE("EventLoop stop looping");
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;

    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);    //防止不属于该EventLoop的Channel进来
    assertInLoopThread();                    //必须在IO线程来做
    poller_->updateChannel(channel);
}

std::pair<int,int> EventLoop::createWakeupFd() {
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
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads {} bytes instead of 1", n);
    }
}

void EventLoop::wakeup() const {
    char one = 1;
    ssize_t n = write(wakeup_fds.second, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes {} bytes instead of 1", n);
    }
}

