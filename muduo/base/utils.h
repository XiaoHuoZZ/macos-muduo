
#ifndef MACOS_MUDUO_BASE_UTILS_H
#define MACOS_MUDUO_BASE_UTILS_H
#include "logger.h"
#include <chrono>
#include <atomic>
#include <time.h>

namespace muduo {
    namespace noncopyable_  // protection from unintended ADL
    {
        class noncopyable
        {
        protected:
            noncopyable() = default;
            ~noncopyable() = default;

        public:  // emphasize the following members are private
            noncopyable( const noncopyable& ) = delete;
            const noncopyable& operator=( const noncopyable& ) = delete;
        };
    }

    using noncopyable = noncopyable_::noncopyable;
    using TimeStamp = uint64_t;
    // 单调时间，基于CLOCK_MONOTONIC, 区分TimeStamp
    using SteadyTime = uint64_t;
    using AtomicBool = std::atomic<bool>;

    namespace time {

        constexpr int64_t MAX_TV_SEC = 18446744073LL;
        constexpr uint64_t MAX_TV_MSEC = 18446744073709ULL;
        constexpr uint64_t MAX_TV_NSEC = 0xFFFFFFFFFFFFFFFFULL;
        constexpr uint32_t NSEC_PER_SEC = 1000000000ULL;

        static TimeStamp getTimeStamp()
        {
            auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
            auto timestamp = tmp.count();
            return timestamp;
        }

        static SteadyTime getSteadyTime()
        {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            // 检查秒数是否超过了纳秒能表达的最大范围 (约 184 亿秒)
            if (ts.tv_sec >= MAX_TV_SEC) {
                // 处理溢出逻辑，返回最大值
                LOG_FATAL("max steady time");
                return 0xFFFFFFFFFFFFFFFFULL;
            }
            return static_cast<uint64_t>(ts.tv_sec) * NSEC_PER_SEC + ts.tv_nsec;
        }

    }


} // namespace muduo
#endif //MACOS_MUDUO_BASE_UTILS_H
