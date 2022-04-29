#include <utility>

#include "muduo/net/tcp_connection.h"
#include "muduo/net/channel.h"
#include "muduo/net/event_loop.h"

using muduo::net::TcpConnection;

TcpConnection::TcpConnection(EventLoop *loop, std::string name, Socket &&socket, const InetAddress &local_addr,
                             const InetAddress &peer_addr)
        : loop_(loop),
          name_(std::move(name)),
          state_(kConnecting),
          socket_(std::move(socket)),
          channel_(std::make_unique<Channel>(loop, socket_.fd())),
          local_addr_(local_addr),
          peer_addr_(peer_addr) {
    /**
     * 设置一些回调
     */
    channel_->setReadCallback([this] { handleRead(); });

    //开启TCP心跳
    socket_.setKeepAlive(true);

    LOG_TRACE("TcpConnection::ctor[{}] at fd = {}", name_, socket_.fd());
}

void TcpConnection::handleRead() {
    char buf[65536];
    ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
    /**
     * 还未实现应用层buffer
     */
    if (n > 0) {
        LOG_INFO("有数据到来");
    }
        //读到了EOF,证明对方关闭了输出流
    else if (n == 0) {
        handleClose();
    } else {
        handleError();
    }
}

TcpConnection::~TcpConnection() {
    LOG_TRACE("TcpConnection::dtor[{}] fd= {} state= {}", name_, channel_->fd(), stateToString());
    assert(state_ == kDisconnected);
}

const char *TcpConnection::stateToString() const {
    switch (state_) {
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnected:
            return "kDisconnected";
        default:
            return "unknown state";
    }
}

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();

    /**
     * 由于TcpConnection在外面被shared_ptr所托管，在TcpConnection内部又想获取这个智能指针
     * 可以使用shared_from_this
     * 不能使用make_shared(this), 因为会干扰资源的生命周期
     */
    connectionCallback_(shared_from_this());
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG_TRACE("TcpConnection::handleClose state = {}", stateToString());
    assert(state_ == kConnected);

    channel_->disableAll();         //是否需要？

    /**
     * 通知TcpServer或TcpClient移除所持有的连接
     */
    closeCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();

    assert(state_ == kConnected);

    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());

    //向EventLoop移除该channel
    channel_->remove();
}

void TcpConnection::handleError() {
    LOG_ERROR("TcpConnection Read Error {}", socket_.fd());
}

void TcpConnection::forceClose() {
    if (state_ == (kConnected | kConnecting)) {
        loop_->runInLoop([this] {
            handleClose();
        });
    }
}
