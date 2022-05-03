/**
 * poller负责开启事件监听, 维护管理ChannelList, 并找出有事件到来的Channel
 */
#ifndef MACOS_MUDUO_POLLER_H
#define MACOS_MUDUO_POLLER_H

#include "muduo/base/utils.h"
#include "muduo/net/eventloop.h"
#include <vector>

struct pollfd;

namespace muduo::net {

    class Channel; //前置声明，避免循环include

    class Poller : noncopyable {
    public:
        using ChannelList = std::vector<Channel *>;

        explicit Poller(EventLoop *loop);

        ~Poller();

        /**
         * 开启监听IO事件
         * 调用fillActiveChannels找出有变化的Channel, 填入到activeChannels
         * 必须在EventLoop线程中调用
         * @param timeoutMs          超时时间
         * @param activeChannels     活跃channel列表
         * @return                   时间戳
         */
        TimeStamp poll(int timeoutMs, ChannelList *activeChannels);

        /**
         * 为该Poller增加channel, 或者更新channel
         * 必须在EventLoop线程中调用
         * @param channel
         */
        void updateChannel(Channel *channel);

        /**
         * 移除该Poller里面的channel
         * @param channel
         */
        void removeChannel(Channel* channel);

        void assertInLoopThread() { owner_loop_->assertInLoopThread(); }

    private:

        /**
         * 负责找出poll_fds_中有变化的fd, 将其所属的channel放入activeChannels里
         * @param numEvents         监听的事件
         * @param activeChannels
         */
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

        using PollFdList = std::vector<struct pollfd>;
        using ChannelMap = std::unordered_map<int, Channel *>;

        EventLoop *owner_loop_;            //所属EventLoop
        PollFdList poll_fds_;              //poll的fd_list
        ChannelMap channels_;              //channel列表   fd<-->channel 用fd当key加快查询,
        // 注意这里存的其实channel的指针，因为channel的声明周期不受Poller控制


    };
}

#endif //MACOS_MUDUO_POLLER_H
