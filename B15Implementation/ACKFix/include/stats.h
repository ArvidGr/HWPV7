#ifndef STATS_H
#define STATS_H

#include <atomic>

class Stats
{
public:
    std::atomic<int> bytes_sent{0};
    std::atomic<int> bytes_received{0};
    std::atomic<int> retransmissions{0};
    std::atomic<int> checksum_errors{0};

    void print();
};

// Globale Statistik-Instanz
extern Stats global_stats;

#endif // STATS_H
