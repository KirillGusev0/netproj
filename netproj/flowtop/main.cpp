#include "flow_monitor.h"
#include <iostream>
#include <csignal>

static bool running = true;
void sigint_handler(int) { running = false; }

int main(int argc, char* argv[]) {
    if (argc < 3 || std::string(argv[1]) != "--iface") {
        std::cerr << "Usage: flowtop --iface <name>\n";
        return 1;
    }
    std::string iface = argv[2];
    signal(SIGINT, sigint_handler);

    try {
        FlowMonitor m(iface);
        m.run();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
