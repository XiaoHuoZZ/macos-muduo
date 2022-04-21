
#ifndef MACOS_MUDUO_BASE_UTILS_H
#define MACOS_MUDUO_BASE_UTILS_H
#include "chrono"
#include "atomic"

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
    using TimeStamp = std::time_t;
    using AtomicBool = std::atomic<bool>;

    namespace time {
        static TimeStamp getTimeStamp()
        {
            std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            auto tmp=std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
            std::time_t timestamp = tmp.count();
            return timestamp;
        }
    }


} // namespace muduo
#endif //MACOS_MUDUO_BASE_UTILS_H
