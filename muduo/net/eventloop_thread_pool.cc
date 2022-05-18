#include "muduo/net/eventloop_thread_pool.h"
#include "muduo/net/eventloop.h"

using muduo::net::EventLoopThreadPool;
using muduo::net::EventLoop;


EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, std::string name)
        : base_loop_(baseLoop),
          started_(false),
          num_threads_(0),
          next_(0),
          name_(std::move(name)) {

}

void EventLoopThreadPool::start() {
    assert(!started_);
    base_loop_->assertInLoopThread();

    started_ = true;

    /**
     * 创建num_threads个线程
     */
    for (int i = 0; i < num_threads_; ++i) {
        auto *t = new EventLoopThread(name_);
        threads_.emplace_back(t);
        loops_.push_back(t->startLoop());
    }


}

EventLoop *EventLoopThreadPool::getNextLoop() {
    base_loop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop = base_loop_;

    if (!loops_.empty())
    {
        loop = loops_[next_++];
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}
