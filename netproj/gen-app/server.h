#pragma once
#include <string>

class TcpServer {
public:
    TcpServer(const std::string& addr, int port);
    void run();

private:
    int listen_fd;
    void setup_socket(const std::string& addr, int port);
};
