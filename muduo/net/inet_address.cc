
#include "muduo/net/inet_address.h"

#include <utility>

using muduo::net::InetAddress;

InetAddress::InetAddress(std::string ipv4, int port)
        : ipv4_(std::move(ipv4)),
          port_(port),
          addr_(createSockaddr()) {

}

InetAddress::InetAddress(int port)
        : ipv4_("0.0.0.0"),
          port_(port),
          addr_(createSockaddr()) {
}

sockaddr_in InetAddress::createSockaddr() {
    struct sockaddr_in tmp{};
    memset(&tmp, 0, sizeof tmp);
    tmp.sin_family = AF_INET;

    if (!inet_aton(ipv4_.data(), &tmp.sin_addr)) {
        LOG_ERROR("ipv4 {} invalid", ipv4_);
    }

    tmp.sin_port = htons(port_);
    return tmp;
}


