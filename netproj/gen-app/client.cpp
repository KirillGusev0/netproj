#include "client.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <random>
#include <vector>
#include <cstring>

namespace {
int make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return fd;
}

int connect_nonblock(const std::string& addr, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    make_nonblocking(fd);

    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, addr.c_str(), &sa.sin_addr);

    connect(fd, (sockaddr*)&sa, sizeof(sa));
    return fd;
}
}

TcpClient::TcpClient(const std::string& addr, int port, int connections, unsigned seed)
    : addr_(addr), port_(port), connections_(connections), seed_(seed) {}

void TcpClient::run() {
    int ep = epoll_create1(0);
    epoll_event ev{}, events[64];
    std::mt19937 rng(seed_);
    std::uniform_int_distribution<int> dist(128, 4096);

    // создаем соединения
    for (int i = 0; i < connections_; i++) {
        int fd = connect_nonblock(addr_, port_);
        if (fd >= 0) {
            ev.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP;
            ev.data.fd = fd;
            epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);
        }
    }

    while (true) {
        int n = epoll_wait(ep, events, 64, -1);
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (events[i].events & EPOLLOUT) {
                std::vector<char> buf(dist(rng), 'x');
                write(fd, buf.data(), buf.size());
                shutdown(fd, SHUT_WR);
            }
            if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                close(fd);
                int newfd = connect_nonblock(addr_, port_);
                if (newfd >= 0) {
                    ev.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP;
                    ev.data.fd = newfd;
                    epoll_ctl(ep, EPOLL_CTL_ADD, newfd, &ev);
                }
            }
        }
    }
}
