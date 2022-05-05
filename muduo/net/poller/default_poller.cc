#include "muduo/net/poller.h"
#include "muduo/net/poller/poll_poller.h"
#include "muduo/net/poller/kqueue_poller.h"

using muduo::net::Poller;
using muduo::net::PollPoller;
using muduo::net::KqueuePoller;

std::unique_ptr<Poller> Poller::newDefaultPoller(EventLoop* loop)
{
    return std::make_unique<KqueuePoller>(loop);
}
