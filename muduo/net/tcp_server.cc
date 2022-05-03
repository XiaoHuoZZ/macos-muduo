#include "muduo/net/tcp_server.h"
#include "muduo/net/eventloop.h"
#include "muduo/net/eventloop_thread_pool.h"
#include "muduo/net/inet_address.h"
#include "muduo/net/tcp_connection.h"
#include "muduo/net/acceptor.h"

using muduo::net::TcpServer;
using muduo::net::Socket, muduo::net::InetAddress;

TcpServer::TcpServer(muduo::net::EventLoop *loop, const InetAddress &listen_addr, std::string name)
        : loop_(loop),
          name_(std::move(name)),
          acceptor_(std::make_unique<Acceptor>(loop, listen_addr)),
          thread_pool_(std::make_unique<EventLoopThreadPool>(loop, name_)),
          started_(false),
          next_conn_id_(1) {
    //设置新连接回调
    acceptor_->setNewConnCallback([this](Socket &&socket, const InetAddress &address) {
        newConnection(std::move(socket), address);
    });
}

TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
    LOG_TRACE("TcpServer::~TcpServer [{}] destructing", name_);

    for (auto &item: connections_) {
        //转移资源所有权
        TcpConnectionPtr conn(std::move(item.second));
        conn->getLoop()->runInLoop(
                [conn] { conn->connectDestroyed(); });
    }
}


void TcpServer::newConnection(Socket &&socket, const InetAddress &peer_addr) {
    loop_->assertInLoopThread();    //需要在IO线程中做，防止对loop的竞争
    ++next_conn_id_;
    std::string conn_name = name_ + std::to_string(next_conn_id_);

    LOG_INFO("TcpServer::newConnection [{}] from {}", conn_name, peer_addr.port());

    /**
     * 从线程池取出一个EventLoop线程
     */
    EventLoop *io_loop = thread_pool_->getNextLoop();

    /**
     * 创建一个新TCP连接
     */
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(io_loop, conn_name, std::move(socket), acceptor_->address(),
                                                            peer_addr);
    connections_[conn_name] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr &ptr) {
        removeConnection(ptr);
    });
    //在IO线程中执行回调
    io_loop->runInLoop([conn] {
        conn->connectEstablished();
    });
}

void TcpServer::start() {
    //注意这里需要使用原子操作exchange, 防止listen多次
    if (!started_.exchange(true)) {

        /**
         * 创建IO线程
         */
        thread_pool_->start();

        assert(!acceptor_->listenning());
        loop_->runInLoop([this] {
            acceptor_->listen();
        });
    }
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    /**
     *  保证removeConnection的动作在主EventLoop线程做
     *  这样可以避免多个线程对removeConnection的竞争
     */
    loop_->runInLoop([this, conn]{
        removeConnectionInLoop(conn);
    });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    loop_->assertInLoopThread();
    LOG_INFO("TcpServer::removeConnection [{}] - connection {}", name_, conn->name());
    size_t n = connections_.erase(conn->name());    //移除连接, 此时会导致conn引用计数减一
    assert(n == 1);
    (void) n;
    /**
     * 这里必须使用queueInLoop
     * 加入队列后，可以让conn的生命周期延长到下一次loop
     * 否则channel 正处于handleEvent阶段时就被析构了
     * 产生生命周期问题
     */
    EventLoop *io_loop = conn->getLoop();
    io_loop->queueInLoop([conn] { conn->connectDestroyed(); });
}

void TcpServer::setThreadNum(int num) {
    thread_pool_->setThreadNum(num);
}


