

#include "muduo/net/eventloop_thread.h"
#include "muduo/net/eventloop.h"
#include <future>

using muduo::net::EventLoopThread;
using muduo::net::EventLoop;

EventLoopThread::EventLoopThread(std::string name)
        : loop_(nullptr),
          name_(std::move(name)),
          exiting_(false) {

}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();     //退出loop循环
        thread_.join();    //thread销毁之前必须join
    }
}

void EventLoopThread::threadFun() {
    EventLoop loop;
    promise_.set_value(&loop);
    loop.loop();
}

EventLoop *EventLoopThread::startLoop() {
    assert(!thread_.joinable());
    auto future = promise_.get_future();
    thread_ = std::thread(&EventLoopThread::threadFun, this);
    //等待EventLoop创建完成
    loop_ =  future.get();
    return loop_;
}




