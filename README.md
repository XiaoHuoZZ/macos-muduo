# macos-muduo
参考学习 muduo [https://github.com/chenshuo/muduo](https://github.com/chenshuo/muduo)

一个基于事件驱动的非阻塞同步C++网络库

- 无其他库依赖，使用C++17标准
- 支持FreeBSD/Darwin/Linux
- 支持poll/kqueue
- 支持多Reactor多线程，线程使用标准库
- 采用runInLoop设计，线程安全容易验证
- 全注释

# 环境

macos / linux

- clang version 13.1.6
- cmake ≥ 3.2.1

# 编译

```bash
./build
```

生成的库文件在lib目录下

# 使用

```cpp
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
```

# 设计要点

[限制并发连接](doc/%E9%99%90%E5%88%B6%E5%B9%B6%E5%8F%91%E8%BF%9E%E6%8E%A5%20bdd97d0d951f4ddc998d073fbadaf4d7.md)

[RunInLoop的设计](doc/RunInLoop%E7%9A%84%E8%AE%BE%E8%AE%A1%203be56a90cc564b01842c99ede4f9b5e3.md)

[socket的一些设置](doc/socket%E7%9A%84%E4%B8%80%E4%BA%9B%E8%AE%BE%E7%BD%AE%20b7d695e4ced044d1abe54a0bacf5e33c.md)

[TCP连接](doc/TCP%E8%BF%9E%E6%8E%A5%20253345d697ae4fc187df252dda77f0c8.md)

[Buffer](doc/Buffer%208a57bcb3cb72443f91f2f824325284c0.md)

[TcpServer多线程的支持](doc/TcpServer%E5%A4%9A%E7%BA%BF%E7%A8%8B%E7%9A%84%E6%94%AF%E6%8C%81%206eb30f2ced854d8ba47eb70edd74d4d1.md)

[Kqueue](doc/Kqueue%20bb937131ff314882a3328e102b76e6f9.md)

[惊群问题](doc/%E6%83%8A%E7%BE%A4%E9%97%AE%E9%A2%98%20f3128f1a77c84ff7934cdf0565b4ed61.md)