#include <iostream>
#include "muduo/net/event_loop.h"
#include "muduo/net/channel.h"
#include <arpa/inet.h>
#include <sys/socket.h>

int createSockServer() {
    int serv_sock;
    int clnt_sock;

    //创建地址信息
    struct sockaddr_in serv_addr{}, clnt_addr{};
    socklen_t clnt_addr_size;

    //创建套接字
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) std::cout << "error" << std::endl;

    //设置服务器地址信息
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //注意网络字节序
    serv_addr.sin_port = htons(9999);

    //给套接字分配地址
    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        std::cout << "bind error" << std::endl;

    //进入等待连接状态，并生成accept队列
    if (listen(serv_sock, 5) == -1) std::cout << "listen error" << std::endl;

    LOG_INFO("开启listen");

    //从accept队列里面取出一个连接，接收
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

    LOG_INFO("建立一个连接");

    return clnt_sock;
}

void testRunInLoop(muduo::net::EventLoop* loop) {
    getchar();     //输入一个字符，看是否能立即响应
    loop->runInLoop([]{
        LOG_INFO("run in loop");
    });
}

int main() {
    spdlog::set_level(spdlog::level::trace); // Set global log level to debug

    muduo::net::EventLoop loop;

    std::thread t(testRunInLoop, &loop);
    t.detach();
    loop.loop();
    return 0;
}
