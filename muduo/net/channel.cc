

#include "muduo/net/channel.h"
#include "muduo/net/event_loop.h"
#include <poll.h>

using muduo::net::Channel;
using muduo::net::EventLoop;

const int Channel::kNoneEvent = 0;                    //poll里面的无事件
const int Channel::kReadEvent = POLLIN | POLLPRI;    //poll里面的读事件   POLLPRI代表高优先级读
const int Channel::kWriteEvent = POLLOUT;            //poll里面的写事件

Channel::Channel(EventLoop *loop, int fd)
        : loop_(loop),
          fd_(fd),
          events_(0),
          revents_(0),
          index_(-1) {
}

void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::handleEvent() {
    //错误事件
    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();
    }
    //可读事件
    if (revents_ & (POLLIN | POLLPRI)) {
        if (readCallback_) readCallback_();
    }
    //可写事件
    if (revents_ & POLLOUT) {
        if (writeCallback_) writeCallback_();
    }
}
