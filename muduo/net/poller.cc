
#include "muduo/net/poller.h"
#include "muduo/net/channel.h"

using muduo::net::Poller;

Poller::Poller(EventLoop *loop) : owner_loop_(loop) {}

Poller::~Poller() = default;
