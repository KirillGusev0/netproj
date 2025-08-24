#pragma once
#include <string>

class TcpClient {
public:
    TcpClient(const std::string& addr, int port, int connections, unsigned seed);
    void run();

private:
    std::string addr_;
    int port_;
    int connections_;
    unsigned seed_;
};
