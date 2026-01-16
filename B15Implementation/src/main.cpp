#include "../include/b15board.h"
#include "../include/protocol.h"
#include "../include/stats.h"
#include <iostream>
#include <cstdlib>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " <board> <mode> [verbose]" << endl;
        cout << "  board:   A oder B (Board-Kennung)" << endl;
        cout << "  mode:    send, receive, oder fullduplex" << endl;
        cout << "  verbose: 0 oder 1 (optional, default: 0)" << endl;
        cout << "\nBeispiel (Half-Duplex):" << endl;
        cout << "  Board A: " << argv[0] << " A send" << endl;
        cout << "  Board B: " << argv[0] << " B receive" << endl;
        cout << "\nBeispiel (Full-Duplex):" << endl;
        cout << "  Board A: " << argv[0] << " A fullduplex" << endl;
        cout << "  Board B: " << argv[0] << " B fullduplex" << endl;
        cout << "\nBeispiel (mit Verbose):" << endl;
        cout << "  Board A: " << argv[0] << " A send 1" << endl;
        return 1;
    }

    string board_id = argv[1];
    string mode = argv[2];
    bool verbose = false;

    if (argc >= 4)
    {
        verbose = (atoi(argv[3]) == 1);
    }

    // Validiere Board-Kennung
    if (board_id != "A" && board_id != "B")
    {
        cerr << "Board muss 'A' oder 'B' sein!" << endl;
        return 1;
    }

    cout << "B15F Full-Duplex Kommunikation" << endl;
    cout << "Mit Checksumme & ARQ" << endl;
    cout << endl;

    cout << "Board: " << board_id << endl;
    cout << "Mode: " << mode << endl;
    if (verbose)
    {
        cout << "Verbose: ON" << endl;
    }
    cout << endl;

    B15Board board(board_id, verbose);

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
