
#ifndef MACOS_MUDUO_INET_ADDRESS_H
#define MACOS_MUDUO_INET_ADDRESS_H

#include <arpa/inet.h>
#include <iostream>
#include "muduo/base/logger.h"

namespace muduo::net {


    class InetAddress {
    public:

        explicit InetAddress(const std::string &ipv4, int port);

        explicit InetAddress(int port);

        std::string ipv4() const;

        int port() const { return ntohs(addr_.sin_port); };

        sockaddr_in *sockaddrIn() { return &addr_; }

        void setAddr(const sockaddr_in &addr) { addr_ = addr; };

    private:
        sockaddr_in addr_;

        /**
         * 创建struct sockaddr_in
         * @return
         */
        static sockaddr_in createStruct(const std::string &ipv4, int port);
    };
}


#endif //MACOS_MUDUO_INET_ADDRESS_H
