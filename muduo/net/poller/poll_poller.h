/**
 * 使用POSIX POLL 实现Poller
 */
#ifndef MACOS_MUDUO_POLLER_H
#define MACOS_MUDUO_POLLER_H

#include "muduo/net/poller.h"
#include <vector>

struct pollfd;

namespace muduo::net {

    class Channel; //前置声明，避免循环include

    class PollPoller : noncopyable,
                       public Poller {
    private:

        using PollFdList = std::vector<struct pollfd>;

        PollFdList poll_fds_;              //poll的fd_list

        /**
        * 负责找出poll_fds_中有变化的fd, 将其所属的channel放入activeChannels里
        * @param numEvents         监听的事件
        * @param activeChannels
        */
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    public:
        using ChannelList = std::vector<Channel *>;

        explicit PollPoller(EventLoop *loop);

        ~PollPoller() override;

        /**
         * 开启监听IO事件
         * 调用fillActiveChannels找出有变化的Channel, 填入到activeChannels
         * 必须在EventLoop线程中调用
         * @param timeoutMs          超时时间
         * @param activeChannels     活跃channel列表
         * @return                   时间戳
         */
        TimeStamp poll(int timeoutMs, ChannelList *activeChannels) override;

        /**
         * 为该Poller增加channel, 或者更新channel
         * 必须在EventLoop线程中调用
         * @param channel
         */
        void updateChannel(Channel *channel) override;

        /**
         * 移除该Poller里面的channel
         * @param channel
         */
        void removeChannel(Channel *channel) override;

    };
}

#endif //MACOS_MUDUO_POLLER_H
