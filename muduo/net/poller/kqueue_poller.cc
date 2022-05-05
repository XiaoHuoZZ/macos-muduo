#include "muduo/net/poller/kqueue_poller.h"
#include "muduo/net/channel.h"
#include <sys/event.h>
#include <err.h>

using muduo::net::KqueuePoller;

namespace {
    const int kNew = -1;       //新channel
    const int kAdded = 1;     //旧channel
}

KqueuePoller::KqueuePoller(EventLoop *loop)
        : Poller(loop),
          kqueue_fd_(kqueue()),
          events_(16) {
    if (kqueue_fd_ == -1) {
        LOG_ERROR("kqueue create failed");
    }
}

void KqueuePoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    assert(numEvents <= events_.size());
    for (int i = 0; i < numEvents; i++) {
        //由于struct kevent里面之前存有channel指针，这里直接取出即可
        auto *channel = static_cast<Channel *>(events_[i].udata);
        int revents = 0;
        /**
         * kqueue的filter不是比特位且与Poll的类型不一致，因此需要手动设置channel的revents
         */
        if (events_[i].filter == EVFILT_READ) {
            revents |= Channel::kReadEvent;
        } else if (events_[i].filter == EVFILT_WRITE) {
            revents |= Channel::kWriteEvent;
        }
        channel->set_revents(revents);
        activeChannels->push_back(channel);
    }
}

muduo::TimeStamp KqueuePoller::poll(int timeoutMs, ChannelList *activeChannels) {
    timeout.tv_sec = timeoutMs / 1000;
    int numEvents = ::kevent(kqueue_fd_, nullptr, 0, events_.data(), static_cast<int>(events_.size()), &timeout);
    TimeStamp now = muduo::time::getTimeStamp();
    if (numEvents > 0) {
        LOG_TRACE("{} events happened", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        //如果当前活动事件填满了events_，events需要进行扩容
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_TRACE("nothing happened");
    } else {
        LOG_ERROR("Kqueue:poll()");
    }
    return now;
}

void KqueuePoller::updateChannel(Channel *channel, int opt) {
    assertInLoopThread();
    LOG_TRACE("fd= {} events= {}", channel->fd(), channel->events());

    /**
     * 在poll中index是用来标记poll_fds中的位置
     * 而在kqueue中index用来表示该channel是否在kqueue的关注列表里
     */
    const int index = channel->index();
    if (index == kNew) {
        //如果该channel是新的
        int fd = channel->fd();
        assert(channels_.find(fd) == channels_.end());
        assert(opt == Channel::kEnWriteOpt || opt == Channel::kEnReadOpt);    //如果新的channel加入到kq, channel的操作必定是这两个

        channels_[fd] = channel;         //加入channels
        kq_events_[fd] = 0;

        channel->set_index(kAdded);

        //该channel已经向kq注册了的事件
        int reg_events = kq_events_[fd];
        if (opt == Channel::kEnReadOpt) {
            update_one(EVFILT_READ, EV_ADD | EV_ENABLE, channel);
            kq_events_[fd] = reg_events | Channel::kReadEvent;               //更新事件
        }
        else {
            update_one(EVFILT_WRITE, EV_ADD | EV_ENABLE, channel);
            kq_events_[fd] = reg_events | Channel::kReadEvent;              //更新事件
        }

    } else {
        //如果channel在channels里面
        int fd = channel->fd();
        (void) fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);

        assert(kq_events_.find(fd) != kq_events_.end());
        int reg_events = kq_events_[fd];

        if (opt == Channel::kDisAllOpt) {
            update(EV_DISABLE, channel);
            kq_events_[fd] = 0;
        }
        else if (opt == Channel::kEnReadOpt) {
            update_one(EVFILT_READ, EV_ADD | EV_ENABLE, channel);
            kq_events_[fd] = reg_events | Channel::kReadEvent;
        }
        else if (opt == Channel::kDisReadOpt) {
            update_one(EVFILT_READ, EV_DISABLE, channel);
            kq_events_[fd] = reg_events & (~Channel::kReadEvent);
        }
        else if (opt == Channel::kEnWriteOpt) {
            update_one(EVFILT_WRITE, EV_ADD | EV_ENABLE, channel);
            kq_events_[fd] = reg_events | Channel::kWriteEvent;
        }
        else if (opt == Channel::kDisWriteOpt) {
            update_one(EVFILT_WRITE, EV_DISABLE, channel);
            kq_events_[fd] = reg_events & (~Channel::kWriteEvent);
        }

    }
}

void KqueuePoller::update(int opt, Channel* channel) {
    int fd = channel->fd();
    assert(kq_events_.find(fd) != kq_events_.end());

    //该channel已经向kq注册了的事件
    int reg_events = kq_events_[fd];

    EventList evs;
    if (reg_events & Channel::kReadEvent) {
        struct kevent tmp{};
        EV_SET(&tmp, channel->fd(), EVFILT_READ, opt, 0, 0, static_cast<void *>(channel));
        evs.push_back(tmp);
    }
    if (reg_events & Channel::kWriteEvent) {
        struct kevent tmp{};
        EV_SET(&tmp, channel->fd(), EVFILT_WRITE, opt, 0, 0, static_cast<void *>(channel));
        evs.push_back(tmp);
    }


    //向kqueue应用更改
    int r = kevent(kqueue_fd_, evs.data(), static_cast<int>(evs.size()), nullptr, 0, nullptr);
    if (r == -1) {
        err(EXIT_FAILURE, "kevent register");
        LOG_ERROR("kqueue::update error");
    }
    for (auto &e: evs) {
        if (e.flags & EV_ERROR) {
            LOG_ERROR("kqueue::update event error {}", e.data);
        }
    }
}

void KqueuePoller::update_one(int event, int opt, Channel* channel) const {
    struct kevent tmp{};
    EV_SET(&tmp, channel->fd(), event, opt, 0, 0, static_cast<void *>(channel));

    //向kqueue应用更改
    int r = kevent(kqueue_fd_, &tmp, 1, nullptr, 0, nullptr);
    if (r == -1) {
        LOG_ERROR("kqueue::update error");
    }
    if (tmp.flags & EV_ERROR) {
        LOG_ERROR("kqueue::update event error {}", tmp.data);
    }

}

void KqueuePoller::removeChannel(muduo::net::Channel *channel) {
    assertInLoopThread();
    LOG_TRACE("poller::removeChannel fd = {}", channel->fd());

    /**
     * 一系列检查
     */
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());                    //必须channel关闭监听事件后才能移除
    int index = channel->index();
    assert(index == kAdded);


    int fd = channel->fd();
    size_t n = channels_.erase(fd);         //Poller移除channel
    (void) n;
    assert(n == 1);

    if (index == kAdded) {
        update(EV_DELETE, channel);
    }
    channel->set_index(kNew);

    n = kq_events_.erase(fd);         //移除kqueue已注册的fd记录
    (void) n;
    assert(n == 1);

}


KqueuePoller::~KqueuePoller() {
    ::close(kqueue_fd_);
}
