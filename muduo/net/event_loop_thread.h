/**
 * 用于在一个新线程启动EventLoop
 */
#ifndef MACOS_MUDUO_EVENT_LOOP_THREAD_H
#define MACOS_MUDUO_EVENT_LOOP_THREAD_H

#include "muduo/base/utils.h"
#include <thread>
#include <future>

namespace muduo::net {

    class EventLoop;      //前向声明，防止嵌套include

    class EventLoopThread : noncopyable {
    private:
        EventLoop* loop_;

        /**
         * 线程执行函数
         */
        void threadFun();

    public:

        /**
         * 开辟一个线程，并启动事件循环
         * @return 指向该EventLoop的指针
         */
        EventLoop* startLoop();
    };
}

#endif //MACOS_MUDUO_EVENT_LOOP_THREAD_H
