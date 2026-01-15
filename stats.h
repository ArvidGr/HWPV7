#ifndef STATS_H
#define STATS_H

// Statistik
class Stats
{
public:
    int bytes_sent = 0;
    int bytes_received = 0;
    int retransmissions = 0;
    int checksum_errors = 0;

    void print();
};

extern Stats global_stats;

#endif // STATS_H
