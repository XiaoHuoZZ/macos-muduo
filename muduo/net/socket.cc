#include "muduo/net/socket.h"
#include "muduo/net/inet_address.h"
#include <unistd.h>
#include <muduo/base/logger.h>
#include <sys/socket.h>
#include <fcntl.h>

using muduo::net::Socket;
using muduo::net::InetAddress;

Socket::Socket() : fd_(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) {
    if (fd_ == -1) {
        LOG_FATAL("create socket failed");
        abort();
    }
//    setNoBlocking();
}


Socket::~Socket() {
    if (::close(fd_) < 0) {
        LOG_ERROR("socket::close error");
    }
}

void Socket::bindAddress(InetAddress &address) {
    if (::bind(fd_, (struct sockaddr *) address.sockaddrIn(), sizeof *address.sockaddrIn()) == -1) {
        LOG_FATAL("socket::bind  error");
        abort();
    }
}

void Socket::listen() {
    //backlog 也就是accept 设置为最大
    if (::listen(fd_, SOMAXCONN) == -1) {
        LOG_FATAL("socket::listen error");
        abort();
    }
}

Socket Socket::accept(InetAddress *peeraddr) {
    sockaddr_in tmp_addr;
    socklen_t size = sizeof(tmp_addr);
    int sock = ::accept(fd_, (struct sockaddr *) &tmp_addr, &size);
    if (sock == -1) {
        LOG_FATAL("socket::accept error");
        abort();
    }
    peeraddr->setAddr(tmp_addr);   //更新地址
    Socket tmp(sock);
    tmp.setNoBlocking();
    return tmp;               //RVO
}

void Socket::setNoBlocking() const {
    int flag = fcntl(fd_, F_GETFL, 0);      //获取之前的设置
    assert(flag);
    if (fcntl(fd_, F_SETFL, flag | O_NONBLOCK) == -1) {
        LOG_FATAL("socket::set-no-blocking error");
        abort();
    }
}




