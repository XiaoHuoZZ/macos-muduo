

#include "muduo/net/channel.h"
#include "muduo/net/eventloop.h"
#include <poll.h>

using muduo::net::Channel;
using muduo::net::EventLoop;

const int Channel::kNoneEvent = 0;                    //poll里面的无事件
const int Channel::kReadEvent = POLLIN | POLLPRI;    //poll里面的读事件   POLLPRI代表高优先级读
const int Channel::kWriteEvent = POLLOUT;            //poll里面的写事件

const int Channel::kEnReadOpt = 1;
const int Channel::kDisReadOpt = 2;
const int Channel::kEnWriteOpt = 3;
const int Channel::kDisWriteOpt = 4;
const int Channel::kDisAllOpt = 5;

Channel::Channel(EventLoop *loop, int fd)
        : loop_(loop),
          fd_(fd),
          events_(0),
          revents_(0),
          index_(-1),
          event_handling_(false),
          added_to_loop_(false) {
}

Channel::~Channel() {
    assert(!event_handling_);        //进行事件分发的时候，channel不会被析构
}

void Channel::update(int opt) {
    added_to_loop_ = true;
    loop_->updateChannel(this, opt);
}

void Channel::handleEvent(TimeStamp receive_time) {
    event_handling_ = true;
    //对方描述符挂起
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        LOG_ERROR("Channel::handle_event() POLLHUP");
        if (closeCallback_) closeCallback_();
    }
    //错误事件
    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();
    }
    //可读事件
    if (revents_ & (POLLIN | POLLPRI)) {
        if (readCallback_) readCallback_(receive_time);
    }
    //可写事件
    if (revents_ & POLLOUT) {
        if (writeCallback_) writeCallback_();
    }
    event_handling_ = false;
}

void Channel::remove() {
    assert(isNoneEvent());
    added_to_loop_ = false;
    loop_->removeChannel(this);
}
