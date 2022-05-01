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

        /**
         * 这里不使用shared_from_this
         * 因为我不认为执行执行runInLoop时TcpConnection会被析构
         */
        loop_->runInLoop([this] {
            //需要再判断一次，因为该lambda函数可能是放入pending队列中延迟执行，防止执行多次handleClose
            if (state_ == kConnected || state_ == kDisconnecting) {
                handleClose();
            }
        });
    }

}

void TcpConnection::shutdown() {
    //exchange防止shutdown多次
    if (state_.exchange(kDisconnecting) == kConnected) {
        loop_->runInLoop([this] {
            shutdownInLoop();
        });
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    /**
     * 即使状态进入了kDisconnecting, 可能还没有真正关闭写入
     * 因为可能output_buffer_中还有数据未发送出去
     */
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::send(const std::string &message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message.data(), message.size());        //省略一次message拷贝
        }
            //否则需要拷贝一份message到IO线程
        else {
            loop_->runInLoop([this, message] {
                sendInLoop(message.data(), message.size());
            });
        }
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
    loop_->assertInLoopThread();
    ssize_t n_wrote = 0;         //记录已写数据长度

    if (state_ == kDisconnected) {
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    //如果是第一次写（未关注写事件并且output_buffer为空）
    if (!channel_->isWriting() && output_buffer_.readableBytes() == 0) {
        n_wrote = ::write(channel_->fd(), data, len);
        if (n_wrote >= 0) {
            //一次未写完
            if (n_wrote < len) {
                LOG_TRACE("write not complete fd = {}", socket_->fd());
            }
        }
            //写入错误
        else {
            n_wrote = 0;
            if (errno != EWOULDBLOCK) {              //EWOULDBLOCK不是错误
                LOG_ERROR("TcpConnection::sendInLoop fd = {}", socket_->fd());
            }
        }
    }

    assert(n_wrote >= 0);
    //说明一次未写完，将未写完的数据放入buffer中
    if (n_wrote < len) {
        output_buffer_.append(static_cast<const char *>(data) + n_wrote, len - n_wrote);
        //开始监听可写事件
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {

        //写入socket fd
        ssize_t n = ::write(channel_->fd(), output_buffer_.peek(), output_buffer_.readableBytes());

        if (n > 0) {
            output_buffer_.retrieve(n);
            //如果缓冲区已经为空,立即关闭监听写事件，防止busy loop
            if (output_buffer_.readableBytes() == 0) {
                channel_->disableWriting();
                //如果现在TCP连接处于KDisConnecting，需要继续关闭流程
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
            else {
                LOG_TRACE("write not complete fd = {}", socket_->fd());
            }
        }
        //写入失败
        else {
            LOG_ERROR("TcpConnection::handleWrite fd = {}", socket_->fd());
        }

    }
    //可能在别的地方关闭了写事件，因一次channel可以分发多个事件
    else {
        LOG_TRACE("Connection fd = {} is down, no more writing", socket_->fd());
    }
}




