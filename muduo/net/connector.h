/**
 * 连接器, 负责为tcp_clinet提供一个连接
 */
#ifndef MACOS_MUDUO_NET_CONNECTOR_H
#define MACOS_MUDUO_NET_CONNECTOR_H

#include "muduo/base/utils.h"
#include "muduo/net/eventloop.h"
#include "muduo/net/inet_address.h"
#include "muduo/net/socket.h"

namespace muduo::net {
class Connector : noncopyable {
  public:
    using NewConnectionCallback = std::function<void(Socket &&)>;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

    void start();
    // 只允许上层感知到断开后调用（TcpClient -> closeCallback）
    void restart();
    void stop();

    InetAddress serverAddr() { return serverAddr_; };

  private:
    enum States : uint8_t { kDisconnected, kConnecting, kConnected };

    void setState(States s) { state_ = s; }
    void startInLoop();
    void stopInLoop();
    void connect();
    // 将connect后的fd扔进loop，以便获取连接结果通知
    void connecting();
    void handleWrite();
    void handleError();
    void removeAndResetChannel();

    EventLoop* loop_;
    InetAddress serverAddr_;
    AtomicBool connect_;
    std::atomic<States> state_;
    std::unique_ptr<Channel> channel_;
    std::unique_ptr<Socket> sock_;

    NewConnectionCallback newConnectionCallback_;
};

}; // namespace muduo::net

#endif // MACOS_MUDUO_NET_CONNECTOR_H