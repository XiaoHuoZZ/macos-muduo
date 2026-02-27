#include "muduo/net/tcp_client.h"
#include "muduo/net/tcp_connection.h"
#include "muduo/net/callbacks.h"
#include <mutex>

using muduo::net::TcpClient;

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name)
    : loop_(loop),
      connector_(std::make_shared<Connector>(loop_, serverAddr)),
      name_(name),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      connect_(true),
      retry_(true),
      nextConnId_(1) {
        // 为Connector设置新连接到来回调
    connector_->setNewConnectionCallback([this](Socket &&sock) { this->newConnection(std::move(sock)); });
}


TcpClient::~TcpClient() {
    // 注意，TcpClient设计为一个临时对象，不要求在Loop线程里进行析构，这会带来一些问题
    // 如果在非loop线程析构的话，当connection close事件到来时，捕获的this指针可能已被销毁
    // TcpClient 是一个重量级的长生命周期管理组件。
    // 它的预期用法是：在服务器/进程启动时创建，在进程终止前夕析构。
    // 原muduo实现做了一些规避，但并不能100%解决问题，因此这里实现省略，假设进程终止前夕析构
    LOG_INFO("TcpClient [{}] dctor", name_);
}

void TcpClient::connect() {
    LOG_INFO("TcpClient [{}] connect to {}:{}", name_, connector_->serverAddr().ipv4(), connector_->serverAddr().port());
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect() {
    LOG_INFO("TcpClient [{}] disconnect to {}:{}", name_, connector_->serverAddr().ipv4(), connector_->serverAddr().port());
    connect_ = false;
    {
        std::unique_lock lock(mutex_);
        if (connection_) {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(Socket&& sock) {
    loop_->assertInLoopThread();
    auto localaddr = sock.getLocalAddr();
    auto peeraddr = sock.getPeerAddr();
    ++nextConnId_;
    std::string connName = std::to_string(nextConnId_) + name_;

    // 构造新的TcpConnection
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, connName, std::move(sock), localaddr,
                                                            peeraddr);
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr& conn) {
        removeConnection(conn);
    });
    {
        std::unique_lock lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::unique_lock lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    /**
     * 这里必须使用queueInLoop
     * 加入队列后，可以让conn的生命周期延长到下一次loop
     * 否则channel 正处于handleEvent阶段时就被析构了
     * 产生生命周期问题
     */
    loop_->queueInLoop([conn] { conn->connectDestroyed(); });

    // 连接断开是否同意重试
    if (retry_ && connect_)
    {
        LOG_INFO("TcpClient::reconnect[{}] to {}:{}",
            name_, connector_->serverAddr().ipv4(), connector_->serverAddr().port());
        connector_->restart();
    }
}