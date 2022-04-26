/**
 * 负责一个连接的数据收发
 */

#ifndef MACOS_MUDUO_TCP_CONNECTION_H
#define MACOS_MUDUO_TCP_CONNECTION_H

#include <memory>
#include <iostream>
#include "muduo/base/utils.h"
#include "muduo/net/socket.h"
#include "muduo/net/inet_address.h"
#include "muduo/net/callbacks.h"

namespace muduo::net {

    class EventLoop;

    class Channel;

    class TcpConnection : noncopyable,
                          public std::enable_shared_from_this<TcpConnection> {
    private:
        /**
         * 连接状态
         */
        enum StateE {
            kConnecting, kConnected, kDisconnected
        };

        EventLoop *loop_;                       //注册到哪个Eventloop
        std::string name_;                     //连接名字
        std::atomic<StateE> state_;            //连接装填
        Socket socket_;                         //持有的socket
        std::unique_ptr<Channel> channel_;
        InetAddress local_addr_, peer_addr_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;

        void setState(StateE s) { state_ = s; }

        /**
         * 处理数据到来
         */
        void handleRead();

    public:

        TcpConnection(EventLoop *loop,
                      std::string name,
                      Socket &&socket,
                      const InetAddress &local_addr,
                      const InetAddress &peer_addr);

        ~TcpConnection();

        void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

        void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

        /**
         * 状态字符串
         * @return
         */
        std::string stateToString() const;

        /**
         * 处理新连接建立后的情况
         * 主要是channel开始监听读事件
         */
        void connectEstablished();
    };

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
}

#endif //MACOS_MUDUO_TCP_CONNECTION_H
