
#include "muduo/net/poller.h"
#include "muduo/net/channel.h"
#include <poll.h>

using muduo::net::Poller;

Poller::Poller(EventLoop *loop) : owner_loop_(loop) {}

Poller::~Poller() = default;

muduo::TimeStamp Poller::poll(int timeoutMs, ChannelList *activeChannels) {
    //开启监听
    int numEvents = ::poll(poll_fds_.data(), poll_fds_.size(), timeoutMs);
    TimeStamp now = muduo::time::getTimeStamp();
    if (numEvents > 0) {
        LOG_TRACE("{} events happened", numEvents);
        fillActiveChannels(numEvents, activeChannels);
    } else if (numEvents == 0) {
        LOG_TRACE("nothing happened");
    } else {
        LOG_ERROR("Poller:poll()");
    }
    return now;
}

void Poller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    for (auto iter = poll_fds_.begin(); iter != poll_fds_.end() && numEvents > 0; ++iter) {
        if (iter->revents <= 0) continue;                           //该事件没有变化，略过
        --numEvents;
        auto map_iter = channels_.find(iter->fd);   //通过fd找到对应的channel
        assert(map_iter != channels_.end());
        Channel *channel = map_iter->second;
        assert(channel->fd() == iter->fd);
        channel->set_revents(iter->revents);                      //更新该channel目前活动的事件
        activeChannels->push_back(channel);                       //加入活动的channel列表
    }
}

void Poller::updateChannel(Channel *channel) {
    assertInLoopThread();
    LOG_TRACE("fd= {} events= {}", channel->fd(), channel->events());

    /**
     * 判断是不是新的channel
     */
    if (channel->index() < 0) {
        //如果是一个新channel, 需要追加poll_fds
        assert(channels_.find(channel->fd()) == channels_.end());

        /**
         * 创建新的pfd
         */
        struct pollfd pfd{};
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());   //设置关心的事件
        pfd.revents = 0;                                      //新的pfd肯定没有活动的事件
        poll_fds_.push_back(pfd);                             //加入poll_fds

        /**
         * 更新channel信息
         */
        int idx = static_cast<int>(poll_fds_.size() - 1);
        channel->set_index(idx);                              //更新channel坐标

        /**
         * 将该channel加入list
         */
        channels_[channel->fd()] = channel;
    }

    else {
        //代表是一个旧channel, 需要更新poll_fds_

        /**
         * 各种检查，通过该channel取出pfd
         */
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(poll_fds_.size()));  //越界检查
        struct pollfd &pfd = poll_fds_[idx];                           //取出pdf
        //如果我们想要poller不关心任何事件，可以将pfd.fd设为负数，为了简单可以将channel的fd取反减1
        //至于为什么减一，是为了防止文件描述符为0的情况，fd为0是无法屏蔽POLLERR事件
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);


        /**
         * 更新pfd
         */
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvent()) {
            //让poller忽略该fd
            pfd.fd = -channel->fd() - 1;
        }
    }
}