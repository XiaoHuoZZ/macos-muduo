
#ifndef MACOS_MUDUO_ACCEPTOR_H
#define MACOS_MUDUO_ACCEPTOR_H

#include "muduo/base/utils.h"

namespace muduo::net {

    class EventLoop;
    class Channel;

    class Acceptor {
    private:

        EventLoop* loop;
        Channel* accept_channel_;
    };
}

#endif //MACOS_MUDUO_ACCEPTOR_H
