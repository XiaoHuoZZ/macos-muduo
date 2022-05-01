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
          socket_(std::make_unique<Socket>(std::move(socket))),
          channel_(std::make_unique<Channel>(loop, socket_->fd())),
          local_addr_(local_addr),
          peer_addr_(peer_addr) {
    /**
     * 设置一些回调
     */
    channel_->setReadCallback([this](TimeStamp receive_time) { handleRead(receive_time); });

    //开启TCP心跳
    socket_->setKeepAlive(true);

    LOG_TRACE("TcpConnection::ctor[{}] at fd = {}", name_, socket_->fd());
}

void TcpConnection::handleRead(TimeStamp receive_time) {
    int savedErrno = 0;
    ssize_t n = input_buffer_.readFd(channel_->fd(), &savedErrno);

    if (n > 0) {
        messageCallback_(shared_from_this(), &input_buffer_, receive_time);
    }
        //读到了EOF,证明对方关闭了输出流
    else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
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
    assert(state_ == kConnected || state_ == kDisconnecting);

    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);

    /**
     * 通知TcpServer或TcpClient移除所持有的连接
     */
    closeCallback_(guardThis);
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();

    /**
     * 这里逻辑与handleClose有重叠
     * 这是因为有可能不经由handleClose 而调用connectDestroyed
     */
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }


    //向EventLoop移除该channel
    channel_->remove();
}

void TcpConnection::handleError() {
    LOG_ERROR("TcpConnection Read Error {}", socket_->fd());
}

void TcpConnection::forceClose() {
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->runInLoop([this] {
            loop_->assertInLoopThread();
            //需要再判断一次，因为该lamda函数可能是放入pending队列中延迟执行
            if (state_ == kConnected || state_ == kDisconnecting) {
                handleClose();
            }
        });
    }

}

void TcpConnection::shutdown() {
    if (state_.exchange(kDisconnecting) == kConnected) {
        loop_->runInLoop([this] {
           loop_->assertInLoopThread();
           if (!channel_->isWriting()) {

           }
        });
    }
}


