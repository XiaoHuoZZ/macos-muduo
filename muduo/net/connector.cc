
#include "muduo/net/connector.h"
#include "logger.h"
#include "muduo/net/channel.h"
#include "muduo/net/socket.h"
#include <cassert>
#include <memory>

using muduo::net::Connector;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop), serverAddr_(serverAddr), connect_(false), state_(kDisconnected) {

    LOG_TRACE("Connctor ctor point to {}", serverAddr_.ipv4());
}

Connector::~Connector() {
    LOG_TRACE("Connctor dctor point to {}", serverAddr_.ipv4());
};

void Connector::start() {
    connect_ = true;
    loop_->runInLoop([this] { startInLoop(); });
}

void Connector::restart() {
    loop_->assertInLoopThread();
    setState(kDisconnected);
    connect_ = true;
    startInLoop();
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop([this] { stopInLoop(); });
}

void Connector::startInLoop() {
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_) {
        connect();
    } else {
        LOG_TRACE("repeat connect");
    }
}

void Connector::stopInLoop() {
    loop_->assertInLoopThread();
    if (state_ == kConnecting) {
        setState(kDisconnected);
        removeAndResetChannel();
        sock_.reset();
    }
}

void Connector::connect() {
    assert(state_ == kDisconnected);
    // 创建socket
    sock_ = std::make_unique<Socket>();
    int ret = sock_->connect(&serverAddr_);
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno) {
    // connect成功或者正在连接，都得扔进loop里面，等待通知
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting();
        break;
    default:
        LOG_ERROR("unexpected err {}", savedErrno);
        sock_.reset();
        break;
    }
}

void Connector::connecting() {
    setState(kConnecting);
    // 为该socket创建一个Channel
    assert(sock_ != nullptr);
    channel_ = std::make_unique<Channel>(loop_, sock_->fd());
    channel_->setWriteCallback([this] { handleWrite(); });
    channel_->setErrorCallback([this] { handleError(); });

    // 注册写事件
    channel_->enableWriting();
}

void Connector::handleWrite() {
    LOG_TRACE("handle write {}", state_);
    assert(sock_ != nullptr);

    if (state_ == kConnecting) {
        removeAndResetChannel();
        // 需要再检查一遍, 来了写事件不一定代表连接成功
        int err = sock_->getSocketErr();
        if (err != 0) {
            LOG_ERROR("Connector handle error {}", err);
            setState(kDisconnected);
            sock_.reset();
        } else {
            // 连接成功，触发回调
            setState(kConnected);
            if (connect_) {
                // 转移Socket所属权给TcpClient
                newConnectionCallback_(std::move(*sock_));
                sock_.reset();
            }
        }
    } else {
        // Connector被关闭了, 此时在loop中state_状态一定是disconnected
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError() {
    LOG_ERROR("Connector::handleErr state={}", state_);
    assert(sock_ != nullptr);

    if (state_ == kConnecting) {
        removeAndResetChannel();
        int err = sock_->getSocketErr();
        LOG_ERROR("err = {}", err);
        setState(kDisconnected);
        sock_.reset();
    }
}

void Connector::removeAndResetChannel() {
    channel_->disableAll();
    channel_->remove();
    // 销毁Channel
    loop_->queueInLoop([this] { this->channel_.reset(); });
}
