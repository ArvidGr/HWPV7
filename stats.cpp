#include "stats.h"
#include <iostream>

using namespace std;

Stats global_stats;

void Stats::print()
{
    cout << "Bytes gesendet:     " << bytes_sent << endl;
    cout << "Bytes empfangen:    " << bytes_received << endl;
    cout << "Wiederholungen:     " << retransmissions << endl;
    cout << "Checksum-Fehler:    " << checksum_errors << endl;
    if (bytes_sent > 0)
    {
        cout << "Fehlerrate:         " << (checksum_errors * 100.0 / bytes_sent) << "%" << endl;
    }
}
