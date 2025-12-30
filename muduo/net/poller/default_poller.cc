#include "muduo/net/poller.h"
#include "muduo/net/poller/poll_poller.h"

#ifndef __linux__
#include "muduo/net/poller/kqueue_poller.h"
#endif //__linux__

using muduo::net::Poller;
using muduo::net::PollPoller;

#ifndef __linux__
using muduo::net::KqueuePoller;
#endif //__linux__

std::unique_ptr<Poller> Poller::newDefaultPoller(EventLoop* loop)
{
#ifdef __linux__
    return std::make_unique<PollPoller>(loop);
#else
    return std::make_unique<KqueuePoller>(loop);
#endif //__linux__
}
