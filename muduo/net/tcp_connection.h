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
#include "muduo/net/buffer.h"

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
        std::unique_ptr<Channel> channel_;      //持有的channel
        InetAddress local_addr_, peer_addr_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        CloseCallback closeCallback_;          //内部使用，用来通知TcpServer移除TcpConnection
        Buffer input_buffer_;                  //输入缓冲

        void setState(StateE s) { state_ = s; }

        /**
         * 处理数据到来
         */
        void handleRead(TimeStamp receive_time);

        /**
         * 主要功能是调用closeCallback_
         * 通知TcpServer/TcpClient移除它们所持有的TcpConnection
         */
        void handleClose();

        /**
         * 处理错误
         */
        void handleError();

    public:

        TcpConnection(EventLoop *loop,
                      std::string name,
                      Socket &&socket,
                      const InetAddress &local_addr,
                      const InetAddress &peer_addr);

        ~TcpConnection();

        /**
         * 设置用户提供的连接回调
         * 当连接建立或者关闭时进行调用
         * @param cb
         */
        void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

        /**
         * 设置用户提供的消息回调
         * 当数据到来时回调
         * @param cb
         */
        void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

        /**
         * 设置连接关闭回调（仅供内部使用）
         * 用于通知TcpServer/TcpCline移除它们所持有的TcpConnection
         * @param cb
         */
        void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

        EventLoop *getLoop() { return loop_; }

        bool connected() const { return state_ == kConnected; }

        bool disconnected() const { return state_ == kDisconnected; }

        /**
         * 状态字符串
         * @return
         */
        const char *stateToString() const;

        std::string name() const { return name_; }

        /**
         * 处理新连接建立后的情况
         * 主要是channel开始监听读事件
         */
        void connectEstablished();

        /**
         * 处理连接关闭前的一些事务
         * 例如取消channel监听，移除channel
         * 这是TcpConnection 析构之前最后执行的一个函数
         */
        void connectDestroyed();

        /**
         * 强制关闭连接
         * 事实上就是调用handleClose
         */
        void forceClose();
    };

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
}

#endif //MACOS_MUDUO_TCP_CONNECTION_H
