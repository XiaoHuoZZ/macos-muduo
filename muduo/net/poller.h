/**
 * poller负责开启事件监听, 维护管理ChannelList, 并找出有事件到来的Channel
 * 这是抽象类
 * 有着不同实现
 */
#ifndef MACOS_MUDUO_POLL_POLLER_H
#define MACOS_MUDUO_POLL_POLLER_H

#include "muduo/base/utils.h"
#include "muduo/net/eventloop.h"
#include <vector>

namespace muduo::net {

    class Channel; //前置声明，避免循环include

    class Poller : noncopyable {
    private:
        EventLoop* owner_loop_;
    protected:
        using ChannelMap = std::unordered_map<int, Channel *>;
        ChannelMap channels_;  //channel列表   fd<-->channel 用fd当key加快查询, 注意持有的是channel的指针, channel的生命周期不受poller控制
    public:
        using ChannelList = std::vector<Channel *>;

        explicit Poller(EventLoop *loop);

        virtual ~Poller();

        /**
         * 开启监听IO事件
         * 将活跃的channel填入到activeChannels
         * 必须在EventLoop线程中调用
         * @param timeoutMs          超时时间
         * @param activeChannels     活跃channel列表
         * @return                   时间戳
         */
        virtual TimeStamp poll(int timeoutMs, ChannelList *activeChannels) = 0;

        /**
         * 为该Poller增加channel, 或者更新channel
         * 必须在EventLoop线程中调用
         * @param channel
         */
        virtual void updateChannel(Channel *channel) = 0;

        /**
         * 移除该Poller里面的channel
         * @param channel
         */
        virtual void removeChannel(Channel* channel) = 0;

        /**
         * 创建默认实现的Poller
         * @param loop
         * @return
         */
        static std::unique_ptr<Poller> newDefaultPoller(EventLoop* loop);

        void assertInLoopThread() { owner_loop_->assertInLoopThread(); }
    };
}

#endif //MACOS_MUDUO_POLL_POLLER_H
