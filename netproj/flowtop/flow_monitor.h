#pragma once
#include <string>
#include <map>
#include <chrono>

struct FlowStats {
    uint64_t packets = 0;
    uint64_t bytes = 0;
    uint64_t payload = 0;
    std::chrono::steady_clock::time_point start;
};

class FlowMonitor {
public:
    FlowMonitor(const std::string& iface);
    void run();

private:
    std::string iface_;
};
