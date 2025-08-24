#include "flow_monitor.h"
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <algorithm>
#include <netinet/if_ether.h>   // для ether_header и ETHERTYPE_IP


struct FlowKey {
    std::string src;
    std::string dst;
    uint16_t sport;
    uint16_t dport;

    bool operator==(const FlowKey& o) const {
        return src == o.src && dst == o.dst && sport == o.sport && dport == o.dport;
    }
};

struct FlowKeyHash {
    std::size_t operator()(const FlowKey& k) const {
        return std::hash<std::string>()(k.src) ^
               (std::hash<std::string>()(k.dst) << 1) ^
               (std::hash<uint16_t>()(k.sport) << 2) ^
               (std::hash<uint16_t>()(k.dport) << 3);
    }
};


static std::unordered_map<FlowKey, FlowStats, FlowKeyHash> flows;
static std::mutex flows_mtx;
static std::atomic<bool> running{true};


FlowMonitor::FlowMonitor(const std::string& iface) : iface_(iface) {}

void FlowMonitor::run() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_live(iface_.c_str(), BUFSIZ, 1, 1000, errbuf);
    if (!handle) throw std::runtime_error("pcap_open_live failed");

    // Отдельный поток — периодический вывод статистики
    std::thread reporter([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::vector<std::pair<FlowKey, FlowStats>> v;
            {
                std::lock_guard<std::mutex> lk(flows_mtx);
                for (auto& kv : flows) v.push_back(kv);
            }
            std::sort(v.begin(), v.end(), [](auto& a, auto& b) {
                return a.second.bytes > b.second.bytes;
            });
            std::cout << "=== TOP FLOWS ===\n";
            for (size_t i = 0; i < v.size() && i < 10; i++) {
                auto& k = v[i].first;
                auto& st = v[i].second;
                std::cout << k.src << ":" << k.sport << " -> "
                          << k.dst << ":" << k.dport
                          << " bytes=" << st.bytes
                          << " payload=" << st.payload
                          << " packets=" << st.packets
                          << "\n";
            }
        }
    });

    // Захват пакетов
    while (running) {
        pcap_pkthdr* header;
        const u_char* packet;
        int res = pcap_next_ex(handle, &header, &packet);
        if (res <= 0) continue;

        const struct ether_header* eth = (struct ether_header*)packet;
        if (ntohs(eth->ether_type) != ETHERTYPE_IP) continue;

        const struct ip* iph = (struct ip*)(packet + sizeof(struct ether_header));
        if (iph->ip_p != IPPROTO_TCP) continue;

        int iphdr_len = iph->ip_hl * 4;
        const struct tcphdr* tcph = (struct tcphdr*)((u_char*)iph + iphdr_len);

        FlowKey k{
            inet_ntoa(iph->ip_src),
            inet_ntoa(iph->ip_dst),
            ntohs(tcph->th_sport),
            ntohs(tcph->th_dport)
        };

        std::lock_guard<std::mutex> lk(flows_mtx);
        auto& st = flows[k];
        if (st.packets == 0) st.start = std::chrono::steady_clock::now();
        st.packets++;
        st.bytes += header->len;
        st.payload += std::max(0, ntohs(iph->ip_len) - iphdr_len - (tcph->th_off * 4));
    }

    reporter.join();
}
