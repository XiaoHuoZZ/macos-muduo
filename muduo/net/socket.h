

#ifndef MACOS_MUDUO_SOCKET_H
#define MACOS_MUDUO_SOCKET_H

#include "muduo/base/utils.h"

namespace muduo::net {

    class InetAddress;

    class Socket : noncopyable {
    private:
        int fd_;
    public:

        /**
         * 创建一个socket
         */
        explicit Socket();

        /**
         * 包装一个已有的socket
         * @param sock
         */
        explicit Socket(int sock) : fd_(sock) {}

        Socket(Socket &&rp) noexcept: fd_(rp.fd_) { rp.fd_ = -1; }

        Socket &operator=(Socket &&rp) noexcept {
            fd_ = rp.fd_;
            rp.fd_ = -1;
            return *this;
        }

        ~Socket();

        int fd() const { return fd_; }

        /**
         * 绑定一个地址
         * @param address 地址
         */
        void bindAddress(InetAddress &address);

        /**
         * 开启监听
         */
        void listen();

        /**
         * 受理一个连接
         * @param peeraddr 客户端地址
         * @return 得到peer socket 作数据传输
         */
        Socket accept(InetAddress *peeraddr);

        /**
         * 设置为非阻塞模式
         */
        void setNoBlocking() const;
    };
}

#endif //MACOS_MUDUO_SOCKET_H
