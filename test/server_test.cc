#include "muduo/net/eventloop.h"
#include "muduo/net/inet_address.h"
#include "muduo/net/socket.h"
#include "muduo/net/acceptor.h"
#include "muduo/net/tcp_server.h"
#include "muduo/net/buffer.h"
#include <arpa/inet.h>
#include <sys/socket.h>


using namespace muduo::net;

void testServer() {
    EventLoop loop;
    InetAddress listen_addr(8080);
    TcpServer server(&loop, listen_addr, "server");
    //设置连接回调
    server.setConnectionCallback([](const TcpConnectionPtr& ptr) {
        if (ptr->connected()) {
            LOG_INFO("connection up");
        }
        else {
            LOG_INFO("connection down");
        }
    });
    //设置消息回调
    server.setMessageCallback([](const TcpConnectionPtr& ptr, Buffer* buffer, muduo::TimeStamp receive_time){
        std::string tmp = buffer->retrieveAllAsString();
        ptr->send(tmp);
    });
    server.setThreadNum(4);  //设置sub-reactor数量
    server.start();
    loop.loop();
}

int main() {

    spdlog::set_level(spdlog::level::trace); // Set global log level to debug

    testServer();

    return 0;
}
