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
    setNoBlocking();
    LOG_TRACE("socket::create {}", fd());
}


Socket::~Socket() {
    if (fd_ != -1) {
        if (::close(fd_) < 0) {
            LOG_ERROR("socket::close error {}", fd());
        }
        LOG_TRACE("socket::close {}", fd());
    }
}

void Socket::bindAddress(const InetAddress &address) const {
    auto tmp = address.sockaddrIn();
    if (::bind(fd_, (struct sockaddr *) &tmp, sizeof tmp) == -1) {
        LOG_FATAL("socket::bind  error {}", fd());
        abort();
    }
}

void Socket::listen() const {
    //backlog 也就是accept 设置为最大
    if (::listen(fd_, SOMAXCONN) == -1) {
        LOG_FATAL("socket::listen error {}", fd());
        abort();
    }
}

Socket Socket::accept(InetAddress *peeraddr) const {
    sockaddr_in tmp_addr{};
    socklen_t size = sizeof(tmp_addr);
    int sock = ::accept(fd_, (struct sockaddr *) &tmp_addr, &size);
    if (sock == -1) {
        LOG_FATAL("socket::accept error {}", fd());
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




