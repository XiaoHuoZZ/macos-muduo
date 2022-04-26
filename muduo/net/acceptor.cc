#include "muduo/net/acceptor.h"
#include "muduo/net/event_loop.h"
#include "muduo/net/inet_address.h"

using muduo::net::Acceptor;

Acceptor::Acceptor(muduo::net::EventLoop *loop, const muduo::net::InetAddress &listen_addr)
        : loop_(loop),
          accept_channel_(loop, accept_socket_.fd()),
          listenning_(false),
          addr_(listen_addr) {
    //设置reuse addr
    accept_socket_.setReuseAddr(true);
    //绑定地址
    accept_socket_.bindAddress(listen_addr);
    //设置channel回调，处理连接到来的事件
    accept_channel_.setReadCallback([this] { handleRead(); });
}

void Acceptor::listen() {
    loop_->assertInLoopThread();             //此操作需要在EventLoop线程上来做，避免对EventLoop有竞争问题
    listenning_ = true;
    accept_socket_.listen();                 //socket开启监听
    accept_channel_.enableReading();        //读事件监听
}

void Acceptor::handleRead() {
    loop_->assertInLoopThread();               //此操作需要在EventLoop线程上来做，避免对EventLoop有竞争问题

    InetAddress peer_add(0);
    Socket conn_sock = accept_socket_.accept(&peer_add);   //建立连接，得到新的socket

    /**
     * 如果没有设置回调，RAII机制会自动关闭conn_sock
     */
    if (newConnCallback_) {
        newConnCallback_(std::move(conn_sock), peer_add);    //这里需要执行移动语义，避免conn_sock被释放
    }
}


