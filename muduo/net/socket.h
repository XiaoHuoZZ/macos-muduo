

#ifndef MACOS_MUDUO_SOCKET_H
#define MACOS_MUDUO_SOCKET_H

#include "muduo/base/utils.h"

namespace muduo::net {

    class InetAddress;

    class Socket : noncopyable {
    private:
        const int fd_;
    public:

        explicit Socket(int sock) : fd_(sock) {}

        ~Socket();

        void bindAddress(const InetAddress& address);

        void listen();

        int accept(InetAddress* peeraddr);
    };
}

#endif //MACOS_MUDUO_SOCKET_H
