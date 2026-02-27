#include "muduo/net/acceptor.h"
#include "muduo/net/eventloop.h"
#include "muduo/net/eventloop_thread.h"
#include "muduo/net/tcp_client.h"
#include "muduo/net/tcp_server.h"
#include <arpa/inet.h>
#include <memory>
#include <sys/socket.h>

using namespace muduo::net;

namespace {
std::atomic<bool> connected{false};
}; // namespace

void testClient(EventLoop *loop, std::shared_ptr<TcpClient> &client) {

    InetAddress server_addr("127.0.0.1", 8080);
    client = std::make_unique<TcpClient>(loop, server_addr, "client");

    // 设置连接回调
    client->setConnectionCallback([](const TcpConnectionPtr& ptr) {
        if (ptr->connected()) {
            LOG_INFO("connection up");
            connected.store(true, std::memory_order_release);
        } else {
            LOG_INFO("connection down");
        }
    });
    // 设置消息回调
    client->setMessageCallback(
        [](const TcpConnectionPtr& ptr, Buffer* buffer, muduo::TimeStamp receive_time) {
            std::string tmp = buffer->retrieveAllAsString();
            LOG_INFO("recv: {}", tmp);
        });
    client->connect();
}

void inputLoop(std::shared_ptr<TcpClient> &client) {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/quit" || line == "/exit") {
            client->disconnect();
            break;
        }

        if (!connected.load(std::memory_order_acquire)) {
            LOG_ERROR("not connected");
            continue;
        }

        auto conn = client->connection();
        conn->send(line);
    }
}

int main() {

    spdlog::set_level(spdlog::level::trace); // Set global log level to debug

    EventLoopThread loopThread("loop thread for client");
    std::shared_ptr<TcpClient> client;

    testClient(loopThread.startLoop(), client);
    inputLoop(client);

    return 0;
}
