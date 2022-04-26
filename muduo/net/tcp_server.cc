#include "muduo/net/tcp_server.h"

#include "muduo/net/event_loop.h"
#include "muduo/net/inet_address.h"
#include "muduo/net/tcp_connection.h"
#include "muduo/net/acceptor.h"

using muduo::net::TcpServer;
using muduo::net::Socket, muduo::net::InetAddress;

TcpServer::TcpServer(muduo::net::EventLoop *loop, const InetAddress &listen_addr, std::string name)
        : loop_(loop),
          name_(std::move(name)),
          acceptor_(std::make_unique<Acceptor>(loop, listen_addr)),
          started_(false),
          next_conn_id_(1) {
    //设置新连接回调
    acceptor_->setNewConnCallback([this](Socket &&socket, const InetAddress &address) {
        newConnection(std::move(socket), address);
    });
}

TcpServer::~TcpServer() {

}



void TcpServer::newConnection(Socket &&socket, const InetAddress &peer_addr) {
    loop_->assertInLoopThread();    //需要在IO线程中做，防止对loop的竞争
    ++next_conn_id_;
    std::string conn_name = name_ + std::to_string(next_conn_id_);

    LOG_INFO("TcpServer::newConnection [{}] from {}", conn_name, peer_addr.port());

    /**
     * 创建一个新TCP连接
     */
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, conn_name, std::move(socket), acceptor_->address(),
                                                            peer_addr);
    connections_[conn_name] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->connectEstablished();
}

void TcpServer::start() {
    //注意这里需要使用原子操作exchange, 防止listen多次
    if (!started_.exchange(true)) {
        assert(!acceptor_->listenning());
        loop_->runInLoop([this]{
            acceptor_->listen();
        });
    }
}

