/**
 * 定义一些回调
 */

#ifndef MACOS_MUDUO_CALLBACKS_H
#define MACOS_MUDUO_CALLBACKS_H

#include <memory>
#include <functional>

namespace muduo::net {
    class TcpConnection;

    class Buffer;

    class Timestamp;

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr &,
                                               Buffer *,
                                               Timestamp)>;
}

#endif //MACOS_MUDUO_CALLBACKS_H
