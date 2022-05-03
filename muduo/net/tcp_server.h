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

    class EventLoopThreadPool;

    class Acceptor;

    class Socket;


    class TcpServer : noncopyable {
    private:

        using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

        EventLoop *loop_;                                    //Acceptor的EventLoop, 也是主EventLoop
        const std::string name_;
        std::unique_ptr<Acceptor> acceptor_;                  //使用指针方式持有Acceptor
        std::unique_ptr<EventLoopThreadPool> thread_pool_;    //持有的多个EventLoop线程，它们专门用于IO数据传输
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;    //写完成回调，用于控制发出速度
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

        /**
         * 在主EventLoop中执行removeConnection
         * @param conn
         */
        void removeConnectionInLoop(const TcpConnectionPtr& conn);

    public:

        TcpServer(EventLoop *loop, const InetAddress &listen_addr, std::string name);

        /**
         * 生命周期由用户控制
         * 析构时，销毁所属连接
         */
        ~TcpServer();

        /**
         * 服务器开启监听
         * 并创建多线程（如果有设置）
         * 线程安全，可以调用多次
         */
        void start();

        void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

        void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

        /**
         * 设置发送缓冲区清空的回调
         * 常用于匹配速度
         * @param cb
         */
        void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

        /**
         * 设置TcpServer线程数量
         * 0 代表没有线程创建  监听连接以及所有其他IO都在主EventLoop中完成
         * >0  创建num个线程及EventLoop
         * @param num
         */
        void setThreadNum(int num);


        /**
         * 移除一个TCP连接
         * @param conn
         */
        void removeConnection(const TcpConnectionPtr &conn);


    };
}

#endif //MACOS_MUDUO_TCP_SERVER_H
