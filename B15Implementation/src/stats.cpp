#include "../include/stats.h"
#include <iostream>

using namespace std;

Stats global_stats;

void Stats::print()
{
    cout << "Bytes gesendet:     " << bytes_sent.load() << endl;
    cout << "Bytes empfangen:    " << bytes_received.load() << endl;
    cout << "Wiederholungen:     " << retransmissions.load() << endl;
    cout << "Checksum-Fehler:    " << checksum_errors.load() << endl;
    if (bytes_sent.load() > 0)
    {
        cout << "Fehlerrate:         "
             << (checksum_errors.load() * 100.0 / bytes_sent.load()) << "%" << endl;
    }
}
