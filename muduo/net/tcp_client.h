/**
 * Tcp client，负责客户端连接，数据的收发
 */

#ifndef MACOS_MUDUO_TCP_CLIENT_H
#define MACOS_MUDUO_TCP_CLIENT_H

#include "muduo/base/utils.h"
#include "muduo/net/callbacks.h"
#include "muduo/net/connector.h"
#include "muduo/net/eventloop.h"
#include "muduo/net/inet_address.h"
#include <mutex>

namespace muduo::net {

class TcpClient : noncopyable {
  public:
    TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name);
    ~TcpClient();

    void connect();
    // 面向Tcp Connection, 关闭连接
    void disconnect();
    // 面向Connector, 停止重试
    void stop();

    TcpConnectionPtr connection() {
        std::unique_lock lock(mutex_);
        return connection_;
    }

    EventLoop* getLoop() const { return loop_; }

    const std::string& name() const { return name_; }

    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }

    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }

    void setWriteCompleteCallback(WriteCompleteCallback cb) {
        writeCompleteCallback_ = std::move(cb);
    }

  private:
    void newConnection(Socket&& sock);
    void removeConnection(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    std::shared_ptr<Connector> connector_;
    std::string name_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    AtomicBool connect_;
    AtomicBool retry_;
    int nextConnId_;
    std::mutex mutex_;
    TcpConnectionPtr connection_;
};

} // namespace muduo::net

#endif // MACOS_MUDUO_TCP_CLIENT_H