#include "server.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>

namespace {
int make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return fd;
}
}

TcpServer::TcpServer(const std::string& addr, int port) {
    setup_socket(addr, port);
}

void TcpServer::setup_socket(const std::string& addr, int port) {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) throw std::runtime_error("socket failed");

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, addr.c_str(), &sa.sin_addr);

    if (bind(listen_fd, (sockaddr*)&sa, sizeof(sa)) < 0)
        throw std::runtime_error("bind failed");

    if (listen(listen_fd, SOMAXCONN) < 0)
        throw std::runtime_error("listen failed");

    make_nonblocking(listen_fd);
}

void TcpServer::run() {
    int ep = epoll_create1(0);
    epoll_event ev{}, events[64];
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(ep, EPOLL_CTL_ADD, listen_fd, &ev);

    std::cout << "[Server] Listening...\n";

    while (true) {
        int n = epoll_wait(ep, events, 64, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == listen_fd) {
                sockaddr_in cli{};
                socklen_t len = sizeof(cli);
                int cfd = accept(listen_fd, (sockaddr*)&cli, &len);
                if (cfd >= 0) {
                    make_nonblocking(cfd);
                    ev.events = EPOLLIN | EPOLLRDHUP;
                    ev.data.fd = cfd;
                    epoll_ctl(ep, EPOLL_CTL_ADD, cfd, &ev);
                }
            } else {
                char buf[4096];
                int r = read(events[i].data.fd, buf, sizeof(buf));
                if (r <= 0) {
                    close(events[i].data.fd);
                }
            }
        }
    }
}
