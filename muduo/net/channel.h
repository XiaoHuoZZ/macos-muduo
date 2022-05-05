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
        using ReadEventCallback = std::function<void(TimeStamp)>;
        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        static const int kEnReadOpt;
        static const int kDisReadOpt;
        static const int kEnWriteOpt;
        static const int kDisWriteOpt;
        static const int kDisAllOpt;
    private:

        EventLoop *loop_;     //注册到哪个EventLoop
        const int fd_;        //当前文件描述符
        int events_;         //关心的IO事件
        int revents_;        //目前活动的事件
        int index_;          //用于记录该channel在Poller的poll_fds_里面的位置
        bool event_handling_;  //channel是否处于事件分发状态
        bool added_to_loop_;   //是否已经添加到EventLoop

        /**
         * 设置的回调  （使用std::function）
         */
        ReadEventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback errorCallback_;
        EventCallback closeCallback_;

        /**
         * 通知EventLoop更新该channel
         */
        void update(int opt);

    public:

        Channel(EventLoop *loop, int fd);

        ~Channel();

        /**
         * 将到来的事件进行分发
         */
        void handleEvent(TimeStamp receiveTime);

        void setReadCallback(const ReadEventCallback &cb) { readCallback_ = cb; }

        void setWriteCallback(const EventCallback &cb) { writeCallback_ = cb; }

        void setErrorCallback(const EventCallback &cb) { errorCallback_ = cb; }

        void setCloseCallback(const EventCallback &cb) { closeCallback_ = cb; }

        int fd() const { return fd_; }

        int events() const { return events_; }

        int revents() const { return revents_; }

        void set_revents(int revents) { revents_ = revents; }

        bool isNoneEvent() const { return events_ == kNoneEvent; }

        /**
         * 为该fd开启读事件, 并通知EventLoop
         */
        void enableReading() {
            events_ |= kReadEvent;
            update(kEnReadOpt);
        }

        void disableReading() {
            events_ &= ~kReadEvent;
            update(kDisReadOpt);
        }

        void enableWriting() {
            events_ |= kWriteEvent;
            update(kEnWriteOpt);
        }

        void disableWriting() {
            events_ &= ~kWriteEvent;
            update(kDisWriteOpt);
        }

        /**
         * 该fd不关心任何事件，并通知EventLoop
         */
        void disableAll() {
            events_ = kNoneEvent;
            update(kDisAllOpt);
        }

        /**
         * 是否目前正在监听可写事件
         * @return
         */
        bool isWriting() const { return events_ & kWriteEvent; }

        /**
         * 是否目前正在监听可读事件
         * @return
         */
        bool isReading() const { return events_ & kReadEvent; }

        int index() const { return index_; }

        /**
         * 设置新坐标
         * @param idx 在Poller channelList里面的坐标
         */
        void set_index(int idx) { index_ = idx; }

        EventLoop *ownerLoop() { return loop_; }

        /**
         * 向EventLoop移除自己
         */
        void remove();

    };
}

#endif //MACOS_MUDUO_CHANNEL_H
