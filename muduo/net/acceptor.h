/**
 * 用于接收新的TCP连接
 * 并且生成连接建立后的socket
 * 连接建立后通过回调通知调用者
 */

#ifndef MACOS_MUDUO_ACCEPTOR_H
#define MACOS_MUDUO_ACCEPTOR_H

#include "muduo/base/utils.h"
#include "muduo/net/channel.h"
#include "muduo/net/socket.h"
#include "muduo/net/inet_address.h"

namespace muduo::net {

    class EventLoop;

    class Channel;

    class Acceptor : noncopyable {
    public:
        using NewConnCallback = std::function<
        void(Socket
        &&socket, const InetAddress &)>;
    private:

        EventLoop *loop_;                    //使用是哪个事件循环器
        Socket accept_socket_;              //持有的socket
        Channel accept_channel_;            //通过该channel注册事件
        NewConnCallback newConnCallback_;   //连接建立后所调用的函数
        bool listenning_;                  //是否已经开启listen
        InetAddress addr_;

        /**
         * 处理新的连接到来
         * 用于建立一个新连接，并进行回调
         */
        void handleRead();

    public:

        /**
         * 构造Acceptor
         * 完成socket的创建， socket描述符向EventLoop注册（通过channel）， socket地址绑定
         * @param loop
         * @param listen_addr
         */
        Acceptor(EventLoop *loop, const InetAddress &listen_addr);

        void setNewConnCallback(const NewConnCallback &cb) { newConnCallback_ = cb; }

        bool listenning() const { return listenning_; }

        InetAddress address() { return addr_; }

        /**
         * 开启监听
         */
        void listen();
    };
}

#endif //MACOS_MUDUO_ACCEPTOR_H
