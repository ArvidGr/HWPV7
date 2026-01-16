#include "../include/b15board.h"
#include "../include/protocol.h"
#include "../include/stats.h"
#include <iostream>
#include <cstdlib>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "Usage: " << argv[0] << " <mode> [verbose]" << endl;
        cout << "  mode:    send, receive, oder fullduplex" << endl;
        cout << "  verbose: 0 oder 1 (optional, default: 0)" << endl;
        cout << "\nBeispiel (Half-Duplex):" << endl;
        cout << "  Rechner 1: " << argv[0] << " send" << endl;
        cout << "  Rechner 2: " << argv[0] << " receive" << endl;
        cout << "\nBeispiel (Full-Duplex):" << endl;
        cout << "  Rechner 1: " << argv[0] << " fullduplex" << endl;
        cout << "  Rechner 2: " << argv[0] << " fullduplex" << endl;
        cout << "\nBeispiel (mit Verbose):" << endl;
        cout << "  Rechner 1: " << argv[0] << " send 1" << endl;
        return 1;
    }

    string mode = argv[1];
    bool verbose = false;

    if (argc >= 3)
    {
        verbose = (atoi(argv[2]) == 1);
    }

    cout << "B15F Full-Duplex Kommunikation" << endl;
    cout << "Mit Checksumme & ARQ" << endl;
    cout << endl;

    cout << "Mode: " << mode << endl;
    if (verbose)
    {
        cout << "Verbose: ON" << endl;
    }
    cout << endl;

    B15Board board("B15", verbose);

    if (mode == "send")
    {
        board.run_sender_mode();
    }
    else if (mode == "receive")
    {
        board.run_receiver_mode();
    }
    else if (mode == "fullduplex")
    {
        board.run_fullduplex_mode();
    }
    else
    {
        cerr << "Mode muss 'send', 'receive' oder 'fullduplex' sein!" << endl;
        return 1;
    }

    return 0;
}
