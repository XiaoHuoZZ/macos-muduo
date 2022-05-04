#include "muduo/net/poller/poll_poller.h"
#include "muduo/net/channel.h"
#include <poll.h>

using muduo::net::PollPoller;

PollPoller::PollPoller(EventLoop *loop) : Poller(loop) {}

PollPoller::~PollPoller() = default;

muduo::TimeStamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    //开启监听poll_fds_
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

void PollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
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

void PollPoller::updateChannel(Channel *channel) {
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
    } else {
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

void PollPoller::removeChannel(muduo::net::Channel *channel) {
    assertInLoopThread();
    LOG_TRACE("poller::removeChannel fd = {}", channel->fd());

    /**
     * 一系列检查
     */
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());                    //必须channel关闭监听事件后才能移除
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(poll_fds_.size()));


    const struct pollfd &pfd = poll_fds_[idx];               //取得pfd
    (void) pfd;                                             //Release模式下会消除assert, 导致pfd没有使用过，这里使用一下，消除编译器警告
    assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
    size_t n = channels_.erase(channel->fd());         //移除channel指针
    assert(n == 1);
    (void) n;


    if (idx == poll_fds_.size() - 1) {
        poll_fds_.pop_back();           //如果需要移除的pfd在数组末尾，直接弹出就好
    } else {
        //否则与数组末尾交换
        int channelAtEnd = poll_fds_.back().fd;
        iter_swap(poll_fds_.begin() + idx, poll_fds_.end() - 1);

        //如果末尾fd小于0，是一个不用关心的fd，需要还原它真实的fd
        if (channelAtEnd < 0) {
            channelAtEnd = -channelAtEnd - 1;
        }
        channels_[channelAtEnd]->set_index(idx);    //设置新的index
        poll_fds_.pop_back();
    }
}
