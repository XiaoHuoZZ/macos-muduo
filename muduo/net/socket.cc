#include "muduo/net/socket.h"
#include <unistd.h>
#include <muduo/base/logger.h>

using muduo::net::Socket;

Socket::~Socket() {
    if (::close(fd_) < 0) {
        LOG_ERROR("socket::close error");
    }
}
