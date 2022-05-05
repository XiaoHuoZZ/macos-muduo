/**
 * Poller kqueue实现
 */

#ifndef MACOS_MUDUO_KQUEUE_POLLER_H
#define MACOS_MUDUO_KQUEUE_POLLER_H

#include "muduo/net/poller.h"

struct kevent;

namespace muduo::net {
    class KqueuePoller : public Poller {
    private:

        using EventList = std::vector<struct kevent>;

        int kqueue_fd_;                               //kqueue描述符
        EventList changes_;                          //改变事件列表
        EventList events_;                           //待接收事件列表
        struct timespec timeout{};                  //用于设置timeout
        std::unordered_map<int, int> kq_events_;    //用于记录某个fd在kqueue注册了哪些事件   fd<->events

        /**
        *  将events_其所属的channel放入activeChannels里
        * @param numEvents         事件数量
        * @param activeChannels
        */
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;


        /**
         * 向kqueue进行操作
         */
        void update(int opt, Channel* channel);

        void update_one(int event, int opt, Channel* channel);

    public:
        using ChannelList = std::vector<Channel *>;

        explicit KqueuePoller(EventLoop *loop);

        ~KqueuePoller() override;

        /**
         * 开启监听IO事件
         * 调用fillActiveChannels填入到activeChannels
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
        void updateChannel(Channel *channel, int opt) override;

        /**
         * 移除该Poller里面的channel
         * @param channel
         */
        void removeChannel(Channel *channel) override;

    };
}


#endif //MACOS_MUDUO_KQUEUE_POLLER_H
