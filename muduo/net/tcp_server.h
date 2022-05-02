/**
 * 管理Acceptor获得的TcpConnection
 */

#ifndef MACOS_MUDUO_TCP_SERVER_H
#define MACOS_MUDUO_TCP_SERVER_H

#include "muduo/base/utils.h"
#include "muduo/net/callbacks.h"
#include "muduo/net/tcp_connection.h"
#include <unordered_map>

namespace muduo::net {

    class InetAddress;

    class EventLoop;

    class Acceptor;

    class Socket;

    class TcpServer : noncopyable {
    private:

        using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

        EventLoop *loop_;                           //Acceptor的eventloop
        const std::string name_;
        std::unique_ptr<Acceptor> acceptor_;        //使用指针方式持有Acceptor
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        AtomicBool started_;
        int next_conn_id_;
        ConnectionMap connections_;                 //所管理的连接，key是连接的名字，每个连接都有一个名字

        /**
         * Acceptor有新连接建立后，会调用此回调
         * 该回调负责创建TcpConnection对象，并加入到ConnectionMap中
         * @param socket
         * @param peer_addr
         */
        void newConnection(Socket &&socket, const InetAddress &peer_addr);

    public:

        TcpServer(EventLoop *loop, const InetAddress &listen_addr, std::string name);

        /**
         * 生命周期由用户控制
         * 析构时，销毁所属连接
         */
        ~TcpServer();

        /**
         * 服务器开启监听
         * 线程安全，可以调用多次
         */
        void start();

        void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

        void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

        /**
         * 移除一个TCP连接
         * @param conn
         */
        void removeConnection(const TcpConnectionPtr& conn);


    };
}

#endif //MACOS_MUDUO_TCP_SERVER_H
