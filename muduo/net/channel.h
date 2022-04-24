/**
 * channel是对文件描述符及其附带事件的封装
 * 负责向EventLoop注册IO事件以及对事件的分发
 * channel 始终服务于同一个fd
 */

#ifndef MACOS_MUDUO_CHANNEL_H
#define MACOS_MUDUO_CHANNEL_H

#include "muduo/base/utils.h"
#include <functional>

namespace muduo::net {
    class EventLoop;

    class Channel : noncopyable {
    public:
        using EventCallback = std::function<void()>;
    private:
        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        EventLoop *loop_;     //注册到哪个EventLoop
        const int fd_;        //当前文件描述符
        int events_;         //关心的IO事件
        int revents_;        //目前活动的事件
        int index_;          //用于记录该channel在Poller的poll_fds_里面的位置

        /**
         * 设置的回调  （使用std::function）
         */
        EventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback errorCallback_;

        /**
         * 通知EventLoop更新该channel
         */
        void update();

    public:

        Channel(EventLoop *loop, int fd);

        /**
         * 将到来的事件进行分发
         */
        void handleEvent();

        void setReadCallback(const EventCallback &cb) { readCallback_ = cb; }

        void setWriteCallback(const EventCallback &cb) { writeCallback_ = cb; }

        void setErrorCallback(const EventCallback &cb) { errorCallback_ = cb; }

        int fd() const { return fd_; }

        int events() const { return events_; }

        void set_revents(int revents) { revents_ = revents; }

        bool isNoneEvent() const { return events_ == kNoneEvent; }

        /**
         * 为该fd开启读事件, 并通知EventLoop
         */
        void enableReading() {
            events_ |= kReadEvent;
            update();
        }

        int index() const { return index_; }

        /**
         * 设置新坐标
         * @param idx 在Poller channelList里面的坐标
         */
        void set_index(int idx) { index_ = idx; }

        EventLoop *ownerLoop() { return loop_; }

    };
}

#endif //MACOS_MUDUO_CHANNEL_H
