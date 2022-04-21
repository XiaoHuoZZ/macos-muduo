/**
 * EventLoop 就是 Reactor 负责监听以及事件分发
 * 每个线程至多一个EventLoop
 * 内存通过Poller开启事件监听循环以及Channel管理
 * 通过Channel进行事件分发
 */


#ifndef MACOS_MUDUO_NET_EVENTLOOP_H
#define MACOS_MUDUO_NET_EVENTLOOP_H

#include "muduo/base/logger.h"
#include "muduo/base/utils.h"


namespace muduo::net {

    using ThreadId = std::thread::id;

    class Channel;                     //前置声明，避免循环include
    class Poller;                      //前置声明，避免循环include

    class EventLoop : noncopyable {
    public:
        using Functor = std::function<void()>;
    private:
        using ChannelList = std::vector<Channel *>;

        AtomicBool looping_;                     //是否已经开启循环
        AtomicBool quit_;                       //标志是可以退出循环, 必须是原子，因为可能有多个线程操作
        const ThreadId thread_id_;            //记录thread id
        std::unique_ptr<Poller> poller_;     //IO复用器  因为持有的是Poller的指针，因此需要unique_ptr进行托管
        ChannelList activeChannels_;        //有事件到来的Channel列表


        AtomicBool calling_pending_functors_;       //标志EventLoop目前正在处于执行挂起的函数的状态
        std::vector<Functor> pending_functors_;    //挂起的待执行函数列表
        std::mutex functors_mutex_;                //互斥访问pending_functors

        std::pair<int,int> wakeup_fds;                 //专门用于唤醒IO线程的文件描述符，由于用pipe实现，因此需要一对描述符
        std::unique_ptr<Channel> wakeup_channel_;    //专门用于唤醒事件注册的的channel

        /**
         * 取出pipe里面的数据(1字节)，防止下次loop被触发
         */
        void handleRead() const;

        /**
         * 创建一个管道
         * @return 一对文件描述符
         */
        std::pair<int,int> createWakeupFd();



    public:

        EventLoop();

        ~EventLoop();

        /**
         * 断言当前线程是EventLoop所在线程
         */
        void assertInLoopThread() const {
            if (!isInLoopThread()) {
                LOG_FATAL("EventLoop is belong to another thread {}", thread_id_);
                abort();
            }
        }

        /**
         * 判断当前是否在EventLoop所属的线程
         * @return
         */
        bool isInLoopThread() const { return thread_id_ == std::this_thread::get_id(); }

        /**
         * 开启事件循环
         */
        void loop();

        /**
         * 终止事件循环
         */
        void quit();

        /**
         * 在IO线程中执行某个函数
         * @param cb 函数
         */
        void runInLoop(const Functor &cb);

        /**
         * 将函数放入队列，在必要的时候唤醒IO线程
         * @param cb 函数
         */
        void queueInLoop(const Functor &cb);

        /**
         * 为该EventLoop 更新一个Channel
         * 实际是通过Poller来管理
         * @param channel
         */
        void updateChannel(Channel *channel);

        /**
         * 唤醒IO线程
         * 实际是向管道写入1字节
         */
        void wakeup() const;
    };
}  // namespace muduo::net

#endif  // MACOS_MUDUO_NET_EVENTLOOP_H
