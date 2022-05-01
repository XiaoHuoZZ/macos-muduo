
#include "muduo/net/inet_address.h"

#include <utility>

using muduo::net::InetAddress;

InetAddress::InetAddress(const std::string& ipv4, int port)
        : addr_(createStruct(ipv4, port)) {

}

InetAddress::InetAddress(int port)
        : addr_(createStruct("0.0.0.0", port)) {
}

sockaddr_in InetAddress::createStruct(const std::string& ipv4, int port) {
    struct sockaddr_in tmp{};
    memset(&tmp, 0, sizeof tmp);

    tmp.sin_family = AF_INET;

    if (!inet_aton(ipv4.data(), &tmp.sin_addr)) {
        LOG_ERROR("ipv4 {} invalid", ipv4);
    }

    tmp.sin_port = htons(port);

    return tmp;
}

std::string InetAddress::ipv4() const {
    char buf[32];
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}


