# RunInLoop的设计

如果我们想要在IO线程执行某个函数，可以使用runInLoop功能，这是一个强大的功能

**采用一个待执行函数队列，队列里面存放的std::function 。并使用互斥锁来保证队列线程安全**

**在Reactor/EventLoop一次循环结束后，会将队列里面的函数一个一个的执行**

具体实现如下：

```cpp
void EventLoop::runInLoop(Functor cb)
{ 
	//如果在IO线程，立即执行
  if (isInLoopThread())
  {
    cb();
  }
	//否则加入队列，并唤醒IO线程来执行
  else
  {
    queueInLoop(std::move(cb));
  }
}
```

# 如何唤醒IO线程

因为此时IO线程可能陷入IO多路复用中的调用阻塞，因此要设法唤醒它

在**Linux**下可以使用eventFd 更加高效

而其他Unix系统：

广泛**的做法是让EventLoop始终监听一个管道，然后我们向该管道写入一个字节便可唤醒**

```cpp
void EventLoop::wakeup() const {
    char one = 1;
    ssize_t n = write(wakeup_fds.second, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::wakeup() writes {} bytes instead of 1", n);
    }
}
```

# 唤醒IO线程的时机

```cpp
void EventLoop::queueInLoop(const Functor &cb) {
    /**
     * 互斥地将函数加入队列
     */
    {
        std::lock_guard lg(functors_mutex_);
        pending_functors_.push_back(cb);
    }

    /**
     * 唤醒
     * 1. 此时非IO线程，需要立即唤醒IO线程来执行pending_functor
     * 2. 若IO线程此时正在执行pending中的functor, 而该functor又调用了queueInLoop，那就得先
     * 提前向管道塞入一个字节，防止之后loop的时候新加入的functor不能及时执行
     */
    if (!isInLoopThread() || calling_pending_functors_) {
        wakeup();
    }
}
```

# runInLoop的意义

其一，**可以让某些操作在IO线程集中去做，避免了线程安全问题**

其二， 放入队列里面的函数具有**延后执行**的特性，利用**queueInLoop**

,将某个资源传通过方法Functor传进队列，**可以将资源生命周期延长到真正执行该Functor时**