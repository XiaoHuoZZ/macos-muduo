/**
 * 用于在一个新线程启动EventLoop
 */
#ifndef MACOS_MUDUO_EVENTLOOP_THREAD_H
#define MACOS_MUDUO_EVENTLOOP_THREAD_H

#include "muduo/base/utils.h"
#include <thread>
#include <future>

namespace muduo::net {

    class EventLoop;      //前向声明，防止嵌套include

    class EventLoopThread : noncopyable {
    private:
        EventLoop* loop_;
        std::string name_;
        bool exiting_;
        std::thread thread_;
        std::promise<EventLoop*> promise_; //用于同步


        /**
         * 线程执行函数
         */
        void threadFun();

    public:

        explicit EventLoopThread(std::string name);

        ~EventLoopThread();

        /**
         * 开辟一个线程，并启动事件循环
         * @return 指向该EventLoop的指针
         */
        EventLoop* startLoop();
    };
}

#endif //MACOS_MUDUO_EVENTLOOP_THREAD_H
