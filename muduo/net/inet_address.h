
#ifndef MACOS_MUDUO_INET_ADDRESS_H
#define MACOS_MUDUO_INET_ADDRESS_H

#include <arpa/inet.h>
#include <iostream>
#include "muduo/base/logger.h"

namespace muduo::net {


    class InetAddress {
    public:

        explicit InetAddress(std::string ipv4, int port);
        explicit InetAddress(int port);

        std::string ipv4() {return ipv4_;}

    private:
        std::string ipv4_;
        int port_;
        struct sockaddr_in addr_;

        /**
         * 创建struct sockaddr_in
         * @return
         */
        sockaddr_in createSockaddr();
    };
}


#endif //MACOS_MUDUO_INET_ADDRESS_H
