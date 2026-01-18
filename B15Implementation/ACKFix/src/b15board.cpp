#include "../include/b15board.h"
#include "../include/protocol.h"
#include "../include/checksum.h"
#include "../include/stats.h"
#include "../include/error_injector.h"
#include <iostream>
#include <fstream>
#include <bitset>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

mutex board_mutex;
atomic<bool> running(true);

B15Board::B15Board(string board_name, bool verb)
    : drv(B15F::getInstance()), name(board_name), verbose(verb)
{
    // DDRA: Bits 0-3 Output (senden), Bits 4-7 Input (empfangen)
    drv.setRegister(&DDRA, 0x0F);

    tx_clock_state = 0;
    tx_ack_state = 0;
    tx_last_received_ack = 0;

    rx_clock_state = 0;
    rx_ack_state = 0;
    rx_last_received_clock = 0;

    drv.delay_ms(200);

    // Initialisiere Ausgänge (Bits 0-3)
    write_output(0);

    drv.delay_ms(100);

    // Lese initiale Zustände vom anderen Board (Bits 7-4)
    uint8_t initial = read_input();
    rx_last_received_clock = initial & CLOCK;
    tx_last_received_ack = initial & ACK;

    cout << "[" << name << "] Initialisiert!" << endl;
    cout << "[" << name << "] DDRA = 0x0F (Bits 0-3: Output, 4-7: Input)" << endl;
}

void B15Board::set_verbose(bool v)
{
    verbose = v;
}

// LOW-LEVEL B15F ACCESS

void B15Board::write_output(uint8_t data)
{
    lock_guard<mutex> lock(board_mutex);
    uint8_t current = drv.getRegister(&PORTA);
    uint8_t new_val = (current & 0xF0) | (data & 0x0F);
    drv.setRegister(&PORTA, new_val);

    if (verbose)
    {
        cout << "  [" << name << "] Write Bits 0-3: " << bitset<4>(data & 0x0F) << endl;
    }
}

uint8_t B15Board::read_input()
{
    lock_guard<mutex> lock(board_mutex);
    uint8_t value = drv.getRegister(&PINA);
    uint8_t upper = (value >> 4) & 0x0F; // Lese Bits 7-4

    // Kabelkreuzung rückgängig machen: PINA[7]->Bit[0], PINA[6]->Bit[1], PINA[5]->Bit[2], PINA[4]->Bit[3]
    uint8_t crossed = 0;
    if (upper & 0x01)    // PINA bit 4
        crossed |= 0x08; // wird zu Bit 3
    if (upper & 0x02)    // PINA bit 5
        crossed |= 0x04; // wird zu Bit 2
    if (upper & 0x04)    // PINA bit 6
        crossed |= 0x02; // wird zu Bit 1
    if (upper & 0x08)    // PINA bit 7
        crossed |= 0x01; // wird zu Bit 0

    if (verbose)
    {
        cout << "  [" << name << "] Read Bits 7-4 (crossed): " << bitset<4>(crossed) << endl;
    }

    return crossed;
}

// PROTOCOL LAYER

bool B15Board::send_2bits(uint8_t data)
{
    data &= 0x03;

    // TX schreibt NUR bits 0-2 (DATA0, DATA1, CLOCK)
    uint8_t output = 0;
    if (data & 0x01)
        output |= DATA0;
    if (data & 0x02)
        output |= DATA1;

    tx_clock_state ^= CLOCK;
    output |= tx_clock_state;

    write_output(output); // TX schreibt DATA+CLOCK auf Bits 0-2

    for (int i = 0; i < 5000; i++)
    {
        uint8_t input = read_input(); // TX liest ACK vom anderen Board (via Bits 7-4)
        uint8_t received_ack = input & ACK;

        if (received_ack != tx_last_received_ack)
        {
            tx_last_received_ack = received_ack;
            return true;
        }

        drv.delay_us(100);
    }

    return false;
}

uint8_t B15Board::receive_2bits()
{
    for (int i = 0; i < 5000; i++)
    {
        uint8_t input = read_input(); // RX liest DATA+CLOCK vom anderen Board (via Bits 7-4)
        uint8_t received_clock = input & CLOCK;

        if (received_clock != rx_last_received_clock)
        {
            rx_last_received_clock = received_clock;

            uint8_t data = 0;
            if (input & DATA0)
                data |= 0x01;
            if (input & DATA1)
                data |= 0x02;

            rx_ack_state ^= ACK;
            // RX schreibt NUR bit 3 (ACK)
            uint8_t output = rx_ack_state;

            write_output(output); // RX schreibt ACK auf Bit 3

            return data;
        }

        drv.delay_us(100);
    }

    return 0xFF;
}

bool B15Board::send_byte_raw(uint8_t byte)
{
    if (!send_2bits((byte >> 0) & 0x03))
        return false;
    if (!send_2bits((byte >> 2) & 0x03))
        return false;
    if (!send_2bits((byte >> 4) & 0x03))
        return false;
    if (!send_2bits((byte >> 6) & 0x03))
        return false;
    return true;
}

uint8_t B15Board::receive_byte_raw()
{
    uint8_t byte = 0;
    uint8_t part;

    part = receive_2bits();
    if (part == 0xFF)
        return 0xFF;
    byte |= (part << 0);

    part = receive_2bits();
    if (part == 0xFF)
        return 0xFF;
    byte |= (part << 2);

    part = receive_2bits();
    if (part == 0xFF)
        return 0xFF;
    byte |= (part << 4);

    part = receive_2bits();
    if (part == 0xFF)
        return 0xFF;
    byte |= (part << 6);

    return byte;
}

// HIGH-LEVEL PROTOCOL

bool B15Board::send_byte_with_checksum(uint8_t byte)
{
    uint8_t checksum = calculate_checksum(byte);

    for (int retry = 0; retry < MAX_RETRIES; retry++)
    {
        if (retry > 0)
        {
            cout << "[" << name << "] WIEDERHOLUNG " << retry << "/" << MAX_RETRIES << endl;
            global_stats.retransmissions++;
        }

        if (verbose)
        {
            cout << "[" << name << "] Sende Byte: '" << (char)byte << "' (0x"
                 << hex << (int)byte << dec << ") + Checksum: 0x"
                 << hex << (int)checksum << dec << endl;
        }

        if (!send_byte_raw(byte))
        {
            cerr << "[" << name << "] Fehler beim Senden!" << endl;
            return false;
        }

        if (!send_byte_raw(checksum))
        {
            cerr << "[" << name << "] Fehler beim Senden der Checksum!" << endl;
            return false;
        }

        uint8_t response = receive_byte_raw();

        if (response == ACK_BYTE)
        {
            if (verbose)
            {
                cout << "[" << name << "] << ACK empfangen! Byte erfolgreich uebertragen." << endl;
            }
            global_stats.bytes_sent++;
            return true;
        }
        else if (response == NACK_BYTE)
        {
            cout << "[" << name << "] << NACK empfangen! Checksum-Fehler, wiederhole..." << endl;
        }
        else
        {
            cout << "[" << name << "] << Unerwartete Antwort: 0x"
                 << hex << (int)response << dec << endl;
        }
    }

    cerr << "[" << name << "] XXX MAX RETRIES erreicht! Uebertragung fehlgeschlagen." << endl;
    return false;
}

uint8_t B15Board::receive_byte_with_checksum()
{
    if (verbose)
    {
        cout << "\n[" << name << "] Warte auf Byte..." << endl;
    }

    uint8_t byte = receive_byte_raw();
    if (byte == 0xFF)
    {
        if (verbose)
        {
            cerr << "[" << name << "] Timeout beim Empfangen!" << endl;
        }
        return 0xFF;
    }

    uint8_t received_byte = error_injector.inject_error(byte);

    uint8_t received_checksum = receive_byte_raw();
    if (received_checksum == 0xFF)
    {
        if (verbose)
        {
            cerr << "[" << name << "] Timeout beim Empfangen der Checksum!" << endl;
        }
        return 0xFF;
    }

    uint8_t expected_checksum = calculate_checksum(received_byte);

    if (verbose)
    {
        cout << "[" << name << "] Empfangen: 0x" << hex << (int)received_byte << dec
             << ", Checksum: 0x" << hex << (int)received_checksum << dec
             << " (erwartet: 0x" << hex << (int)expected_checksum << dec << ")" << endl;
    }

    if (received_checksum == expected_checksum)
    {
        if (verbose)
        {
            cout << "[" << name << "] >> Checksum OK! Sende ACK." << endl;
        }
        send_byte_raw(ACK_BYTE);
        global_stats.bytes_received++;
        return received_byte;
    }
    else
    {
        cout << "[" << name << "] XX Checksum FEHLER! Sende NACK." << endl;
        send_byte_raw(NACK_BYTE);
        global_stats.checksum_errors++;
        return 0xFF;
    }
}

// OPERATION MODES

void B15Board::run_sender_mode()
{
    cout << "\n[" << name << "] SENDER MODE (Half-Duplex)" << endl;
    cout << "[" << name << "] Gib Nachrichten ein (eine pro Zeile):" << endl;
    cout << "[" << name << "] Jede Zeile wird komplett gesendet + EOT" << endl;

    string line;
    while (getline(cin, line))
    {
        cout << "[" << name << "] Sende Nachricht: \"" << line << "\"" << endl;

        for (char c : line)
        {
            if (!send_byte_with_checksum(c))
            {
                cerr << "[" << name << "] Uebertragung abgebrochen!" << endl;
                global_stats.print();
                return;
            }
        }

        if (!send_byte_with_checksum('\n'))
        {
            cerr << "[" << name << "] Uebertragung abgebrochen!" << endl;
            global_stats.print();
            return;
        }

        cout << "[" << name << "] Sende EOT (End of Transmission)" << endl;
        if (!send_byte_with_checksum(EOT_BYTE))
        {
            cerr << "[" << name << "] Uebertragung abgebrochen!" << endl;
            global_stats.print();
            return;
        }

        cout << "[" << name << "] >>> Nachricht komplett gesendet! <<<\n"
             << endl;
    }

    global_stats.print();
}

void B15Board::run_receiver_mode()
{
    cout << "\n[" << name << "] RECEIVER MODE (Half-Duplex)" << endl;
    cout << "[" << name << "] Warte auf Nachrichten (bis EOT)..." << endl;

    string received_message = "";

    while (true)
    {
        uint8_t byte = receive_byte_with_checksum();

        if (byte == 0xFF)
        {
            continue;
        }

        if (byte == EOT_BYTE)
        {
            cout << "[" << name << "] EOT empfangen! Nachricht komplett." << endl;
            cout << "[" << name << "] EMPFANGENE NACHRICHT:" << endl;
            cout << ">>> " << received_message << " <<<" << endl;
            cout << endl;

            received_message = "";
            continue;
        }

        received_message += (char)byte;
        if (verbose)
        {
            cout << "[" << name << "] Zeichen empfangen: '" << (char)byte
                 << "' (Message bisher: \"" << received_message << "\")" << endl;
        }
    }
}

// FULL-DUPLEX THREADS

void B15Board::sender_thread()
{
    cout << "\n[" << name << " TX] SENDER-THREAD gestartet" << endl;
    cout << "[" << name << " TX] Gib Nachrichten ein:" << endl;

    string line;
    while (running && getline(cin, line))
    {
        cout << "[" << name << " TX] >>> Sende: \"" << line << "\"" << endl;

        for (char c : line)
        {
            if (!send_byte_with_checksum(c))
            {
                cerr << "[" << name << " TX] Fehler!" << endl;
                return;
            }
        }

        send_byte_with_checksum('\n');
        send_byte_with_checksum(EOT_BYTE);

        cout << "[" << name << " TX] Nachricht gesendet!\n"
             << endl;
    }
}

void B15Board::receiver_thread()
{
    cout << "[" << name << " RX] RECEIVER-THREAD gestartet" << endl;

    string filename = "received_" + name + ".txt";
    ofstream outfile(filename, ios::app);

    if (outfile.is_open())
    {
        cout << "[" << name << " RX] Schreibe empfangene Nachrichten in: "
             << filename << endl;
    }

    string received_message = "";

    while (running)
    {
        uint8_t byte = receive_byte_with_checksum();

        if (byte == 0xFF)
        {
            drv.delay_ms(10);
            continue;
        }

        if (byte == EOT_BYTE)
        {
            cout << "[" << name << " RX] EMPFANGEN: \"" << received_message << "\"" << endl;

            if (outfile.is_open())
            {
                outfile << received_message << endl;
                outfile.flush();
            }

            received_message = "";
            continue;
        }

        received_message += (char)byte;
    }

    if (outfile.is_open())
    {
        outfile.close();
    }
}

void B15Board::run_fullduplex_mode()
{
    cout << "\n[" << name << "] PING-PONG FULL-DUPLEX MODE" << endl;
    cout << "[" << name << "] Turn-taking mit NO_DATA_BYTE" << endl;
    cout << "[" << name << "] Empfangene Nachrichten werden in Datei geschrieben." << endl;
    cout << "[" << name << "] Board " << name << " startet als "
         << (name == "A" ? "SENDER" : "RECEIVER") << "\n"
         << endl;

    // Lese alle stdin Daten in Queue
    queue<char> send_queue;
    string line;
    while (getline(cin, line))
    {
        for (char c : line)
        {
            send_queue.push(c);
        }
        send_queue.push('\n');
        send_queue.push(EOT_BYTE);
    }

    cout << "[" << name << "] " << send_queue.size() << " Zeichen zum Senden bereit." << endl;

    // Output file für empfangene Nachrichten
    string filename = "received_" + name + ".txt";
    ofstream outfile(filename, ios::app);
    if (outfile.is_open())
    {
        cout << "[" << name << "] Schreibe empfangene Nachrichten in: " << filename << endl;
    }

    string received_message = "";
    int round = 0;
    bool other_has_data = true; // Annahme: anderes Board hat initial Daten

    // Turn-taking Loop: A sendet → B empfängt → B sendet → A empfängt → ...
    while (other_has_data || !send_queue.empty())
    {
        round++;

        if ((name == "A" && round % 2 == 1) || (name == "B" && round % 2 == 0))
        {
            // SENDER-Turn
            if (!send_queue.empty())
            {
                char c = send_queue.front();
                send_queue.pop();

                if (verbose)
                {
                    cout << "[" << name << " R" << round << "] Sende: '" << c << "' (0x"
                         << hex << (int)(uint8_t)c << dec << ")" << endl;
                }

                if (!send_byte_with_checksum((uint8_t)c))
                {
                    cerr << "[" << name << "] Fehler beim Senden!" << endl;
                    break;
                }
            }
            else
            {
                // Keine Daten mehr - sende NO_DATA_BYTE
                if (verbose)
                {
                    cout << "[" << name << " R" << round << "] Sende: NO_DATA_BYTE" << endl;
                }

                if (!send_byte_with_checksum(NO_DATA_BYTE))
                {
                    cerr << "[" << name << "] Fehler beim Senden von NO_DATA!" << endl;
                    break;
                }
            }
        }
        else
        {
            // RECEIVER-Turn
            uint8_t byte = receive_byte_with_checksum();

            if (byte == 0xFF)
            {
                cout << "[" << name << " R" << round << "] Timeout - anderes Board antwortet nicht!" << endl;
                break;
            }

            if (byte == NO_DATA_BYTE)
            {
                // Anderes Board hat keine Daten mehr
                if (verbose)
                {
                    cout << "[" << name << " R" << round << "] << NO_DATA empfangen" << endl;
                }
                other_has_data = false;
            }
            else if (byte == EOT_BYTE)
            {
                cout << "[" << name << " R" << round << "] << EOT empfangen - Nachricht: \""
                     << received_message << "\"" << endl;

                if (outfile.is_open())
                {
                    outfile << received_message << endl;
                    outfile.flush();
                }

                received_message = "";
            }
            else
            {
                // Normales Daten-Byte
                received_message += (char)byte;
                if (verbose)
                {
                    cout << "[" << name << " R" << round << "] << Empfangen: '" << (char)byte << "'" << endl;
                }
            }
        }
    }

    if (outfile.is_open())
    {
        outfile.close();
    }

    cout << "\n[" << name << "] Fullduplex beendet" << endl;
    global_stats.print();
}
