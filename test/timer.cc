#include <atomic>
#include "logger.h"
#include "muduo/net/eventloop.h"
#include "muduo/net/eventloop_thread.h"
#include "utils.h"


using namespace muduo::net;

std::atomic_int cnt = 0;
TimerId timer3Id(nullptr);
TimerId timer4Id(nullptr);
EventLoop *loop = nullptr;


int main() {
    spdlog::set_level(spdlog::level::trace); // Set global log level to debug

    EventLoopThread eventLoopThread("timer_test");
    loop = eventLoopThread.startLoop();

    auto time = muduo::time::getTimeStamp();
    time += 1 * 60 * 1e9;

    LOG_TRACE("start timer1");
    loop->runAt(time, []() {
        LOG_TRACE("timer1 ring");
    });

    LOG_TRACE("start timer2");
    loop->runAfter(30 * 1000, []() {
        LOG_TRACE("timer2 ring");
    });

    LOG_TRACE("start timer3");
    timer3Id = loop->runAfter(40 * 1000, []() {
        LOG_TRACE("timer3 ring");
    });

    LOG_TRACE("start timer4");
    timer4Id = loop->runEvery(11 * 1000, []() {
        cnt++;
        LOG_TRACE("timer4 ring");
        if (cnt == 4) {
            LOG_TRACE("cancel timer3&4 when timer4 ring 4 times");
            loop->cancel(timer3Id);
            loop->cancel(timer4Id);
        }
    });

    pause();
    return 0;
}
