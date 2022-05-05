# TcpServer多线程的支持

TcpServer中持有一个线程池`thread_pool_`，**每个线程被EventLoopThread对象所封装**

# EventLoopThread

线程池中的每个线程都拥有一个EventLoop并开启了循环，每个EventLoopThread相关逻辑如下：

```cpp
//线程执行的函数
void EventLoopThread::threadFun() {
    EventLoop loop;
    promise_.set_value(&loop);
    loop.loop();
}

//创建线程并开启循环
EventLoop *EventLoopThread::startLoop() {
    assert(!thread_.joinable());
    auto future = promise_.get_future();
    //启动线程
    thread_ = std::thread(&EventLoopThread::threadFun, this);
    //等待threadFun中EventLoop创建完成
    loop_ =  future.get();
    return loop_;
}
```

这里需要注意一点，创建线程并启动循环**需要用线程同步来保证startLoop()函数能得到正确的loop指针,**  这里我使用的是标准库的std::promise

# 主-从Reactor

TcpServer自己拥有一个EventLoop给Acceptor组件用，**专门用于监听连接的到来**。这个EventLoop也称为**main-reactor**

当连接建立后，通过`thread_pool_->getNextLoop()`

按轮盘转的原则从线程池中挑出一个EventLoop来**专门用于IO数据的收发，**该EventLoop也称为**sub-reactor**

并创建TCP连接

```cpp
/**
 * 从线程池取出一个EventLoop线程
 */
EventLoop *io_loop = thread_pool_->getNextLoop();

/**
 * 创建一个新TCP连接
 */
TcpConnectionPtr conn = std::make_shared<TcpConnection>(io_loop, conn_name, std::move(socket), acceptor_->address(),
                                                            peer_addr);
```

由于采用了one loop per thread模型，因此想要使用多reactor必然使用多线程