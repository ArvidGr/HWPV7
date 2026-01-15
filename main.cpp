#include "b15simulator.h"
#include "error_injector.h"
#include <iostream>
#include <cstdlib>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " <board> <mode> [error_rate]" << endl;
        cout << "  board:      A oder B" << endl;
        cout << "  mode:       send, receive, oder fullduplex" << endl;
        cout << "  error_rate: 0-100 (optional, default: 0)" << endl;
        cout << "\nBeispiel (Half-Duplex):" << endl;
        cout << "  Terminal 1: " << argv[0] << " A send" << endl;
        cout << "  Terminal 2: " << argv[0] << " B receive 20" << endl;
        cout << "\nBeispiel (Full-Duplex):" << endl;
        cout << "  Terminal 1: " << argv[0] << " A fullduplex" << endl;
        cout << "  Terminal 2: " << argv[0] << " B fullduplex 20" << endl;
        cout << "               (20% Fehlerrate zum Testen)" << endl;
        return 1;
    }

    char board = argv[1][0];
    string mode = argv[2];
    int error_rate = 0;

    if (argc >= 4)
    {
        error_rate = atoi(argv[3]);
        if (error_rate < 0 || error_rate > 100)
        {
            cerr << "Fehlerrate muss zwischen 0 und 100 sein!" << endl;
            return 1;
        }
    }

    if (board != 'A' && board != 'B')
    {
        cerr << "Board muss 'A' oder 'B' sein!" << endl;
        return 1;
    }

    bool is_a = (board == 'A');

    cout << "B15F Simulator mit Checksumme & ARQ" << endl;
    cout << "Board " << board << " - " << (mode == "send" ? "SENDER    " : "EMPFAENGER") << endl;
    if (error_rate > 0)
    {
        cout << "Fehlerrate: " << error_rate << "%" << endl;
    }

    if (error_rate > 0)
    {
        error_injector.set_error_rate(error_rate);
    }

    B15Simulator board_sim(is_a, false);

    if (mode == "send")
    {
        board_sim.run_sender_mode();
    }
    else if (mode == "receive")
    {
        board_sim.run_receiver_mode();
    }
    else if (mode == "fullduplex")
    {
        board_sim.run_fullduplex_mode();
    }
    else
    {
        cerr << "Mode muss 'send', 'receive' oder 'fullduplex' sein!" << endl;
        return 1;
    }

    return 0;
}
