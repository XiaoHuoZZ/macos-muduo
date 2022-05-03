/**
 * 线程池
 * 每个线程均含有一个EventLoop
 */

#ifndef MACOS_MUDUO_EVENT_LOOP_THREAD_POOL_H
#define MACOS_MUDUO_EVENT_LOOP_THREAD_POOL_H

#include "muduo/base/utils.h"
#include "muduo/net/eventloop_thread.h"
#include <memory>
#include <vector>
#include <string>

namespace muduo::net {

    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool : noncopyable {
    private:
        EventLoop* base_loop_;      //main EventLoop 用来监听连接请求的
        bool started_;
        int num_threads_;           //线程个数
        int next_;                  //loops_中的坐标
        std::string name_;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> loops_;

    public:
        explicit EventLoopThreadPool(EventLoop* base_loop, std::string name);

        ~EventLoopThreadPool() = default;

        void setThreadNum(int num) {
            num_threads_ = num;
        }

        /**
         * 启动num_threads_个线程并开启循环
         */
        void start();

        /**
         * 依次从loops_中挑选出一个EventLoop
         * 如果是单线程服务，每次获得的都是base_loop
         * @return
         */
        EventLoop* getNextLoop();
    };

}

#endif //MACOS_MUDUO_EVENT_LOOP_THREAD_POOL_H
