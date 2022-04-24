

#include "muduo/net/event_loop_thread.h"
#include "muduo/net/event_loop.h"
#include <future>

using muduo::net::EventLoopThread;
using muduo::net::EventLoop;

void EventLoopThread::threadFun() {

}

EventLoop *EventLoopThread::startLoop() {
    return nullptr;
}
