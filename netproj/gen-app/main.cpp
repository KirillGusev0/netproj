#include "server.h"
#include "client.h"
#include <iostream>
#include <string>
#include <csignal>

bool running = true;
void sigint_handler(int) { running = false; }

int main(int argc, char* argv[]) {
    std::string addr = "127.0.0.1";
    int port = 8000;
    std::string mode;
    int connections = 1;
    unsigned seed = 42;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--addr" && i+1 < argc) {
            std::string s = argv[++i];
            auto pos = s.find(':');
            addr = s.substr(0,pos);
            port = std::stoi(s.substr(pos+1));
        } else if (a == "--mode" && i+1 < argc) {
            mode = argv[++i];
        } else if (a == "--connections" && i+1 < argc) {
            connections = std::stoi(argv[++i]);
        } else if (a == "--seed" && i+1 < argc) {
            seed = std::stoul(argv[++i]);
        }
    }

    signal(SIGINT, sigint_handler);

    if (mode == "server") {
        TcpServer s(addr, port);
        s.run();
    } else if (mode == "client") {
        TcpClient c(addr, port, connections, seed);
        c.run();
    } else {
        std::cerr << "Usage: --addr host:port --mode server|client [--connections N] [--seed X]\n";
        return 1;
    }
}
