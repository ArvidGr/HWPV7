#include "b15simulator.h"
#include "protocol.h"
#include "checksum.h"
#include "error_injector.h"
#include "stats.h"
#include <iostream>
#include <fstream>
#include <bitset>
#include <thread>
#include <chrono>

using namespace std;

B15Simulator::B15Simulator(bool is_a, bool verb) : is_board_a(is_a), verbose(verb)
{
    name = is_a ? "Board A" : "Board B";

    current_clock_state = 0;
    current_ack_state = 0;

    this_thread::sleep_for(chrono::milliseconds(200));

    uint8_t initial = read_input();
    last_received_ack = initial & ACK;
    last_received_clock = initial & CLOCK;

    write_output(0);

    cout << "[" << name << "] Initialisiert!" << endl;
}

void B15Simulator::write_output(uint8_t data)
{
    if (is_board_a)
    {
        cable.write_bits_a(data & 0x0F);
        if (verbose)
        {
            cout << "  [" << name << "] -> " << bitset<4>(data & 0x0F) << endl;
        }
    }
    else
    {
        cable.write_bits_b(data & 0x0F);
        if (verbose)
        {
            cout << "  [" << name << "] -> " << bitset<4>(data & 0x0F) << endl;
        }
    }
}

uint8_t B15Simulator::read_input()
{
    if (is_board_a)
    {
        return cable.read_bits_b(); // Board A reads from Board B's bits
    }
    else
    {
        return cable.read_bits_a(); // Board B reads from Board A's bits
    }
}

bool B15Simulator::send_2bits(uint8_t data)
{
    data &= 0x03;

    uint8_t output = current_ack_state;
    if (data & 0x01)
        output |= DATA0;
    if (data & 0x02)
        output |= DATA1;

    current_clock_state ^= CLOCK;
    output |= current_clock_state;

    write_output(output);

    for (int i = 0; i < 5000; i++)
    {
        uint8_t input = read_input();
        uint8_t received_ack = input & ACK;

        if (received_ack != last_received_ack)
        {
            last_received_ack = received_ack;
            return true;
        }

        this_thread::sleep_for(chrono::microseconds(100));
    }

    return false;
}

uint8_t B15Simulator::receive_2bits()
{
    for (int i = 0; i < 5000; i++)
    {
        uint8_t input = read_input();
        uint8_t received_clock = input & CLOCK;

        if (received_clock != last_received_clock)
        {
            last_received_clock = received_clock;

            uint8_t data = 0;
            if (input & DATA0)
                data |= 0x01;
            if (input & DATA1)
                data |= 0x02;

            current_ack_state ^= ACK;
            uint8_t output = current_ack_state | current_clock_state;

            write_output(output);

            return data;
        }

        this_thread::sleep_for(chrono::microseconds(100));
    }

    return 0xFF;
}

bool B15Simulator::send_byte_raw(uint8_t byte)
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

uint8_t B15Simulator::receive_byte_raw()
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

bool B15Simulator::send_byte_with_checksum(uint8_t byte)
{
    uint8_t checksum = calculate_checksum(byte);

    for (int retry = 0; retry < MAX_RETRIES; retry++)
    {
        if (retry > 0)
        {
            cout << "[" << name << "] WIEDERHOLUNG " << retry << "/" << MAX_RETRIES << endl;
            global_stats.retransmissions++;
        }

        cout << "[" << name << "] Sende Byte: '" << (char)byte << "' (0x"
             << hex << (int)byte << dec << ") + Checksum: 0x"
             << hex << (int)checksum << dec << endl;

        // Sende Daten-Byte
        if (!send_byte_raw(byte))
        {
            cerr << "[" << name << "] Fehler beim Senden!" << endl;
            return false;
        }

        // Sende Checksum
        if (!send_byte_raw(checksum))
        {
            cerr << "[" << name << "] Fehler beim Senden der Checksum!" << endl;
            return false;
        }

        // Warte auf ACK/NACK
        uint8_t response = receive_byte_raw();

        if (response == ACK_BYTE)
        {
            cout << "[" << name << "] << ACK empfangen! Byte erfolgreich uebertragen." << endl;
            global_stats.bytes_sent++;
            return true;
        }
        else if (response == NACK_BYTE)
        {
            cout << "[" << name << "] << NACK empfangen! Checksum-Fehler, wiederhole..." << endl;
        }
        else
        {
            cout << "[" << name << "] << Unerwartete Antwort: 0x" << hex << (int)response << dec << endl;
        }
    }

    cerr << "[" << name << "] XXX MAX RETRIES erreicht! Uebertragung fehlgeschlagen." << endl;
    return false;
}

uint8_t B15Simulator::receive_byte_with_checksum()
{
    cout << "\n[" << name << "] Warte auf Byte..." << endl;

    // Empfange Daten-Byte (mit Fehler-Injektion!)
    uint8_t byte = receive_byte_raw();
    if (byte == 0xFF)
    {
        cerr << "[" << name << "] Timeout beim Empfangen!" << endl;
        return 0xFF;
    }

    // Fehler-Injektion hier!
    uint8_t received_byte = error_injector.inject_error(byte);

    // Empfange Checksum
    uint8_t received_checksum = receive_byte_raw();
    if (received_checksum == 0xFF)
    {
        cerr << "[" << name << "] Timeout beim Empfangen der Checksum!" << endl;
        return 0xFF;
    }

    // Berechne erwartete Checksum
    uint8_t expected_checksum = calculate_checksum(received_byte);

    cout << "[" << name << "] Empfangen: 0x" << hex << (int)received_byte << dec
         << ", Checksum: 0x" << hex << (int)received_checksum << dec
         << " (erwartet: 0x" << hex << (int)expected_checksum << dec << ")" << endl;

    if (received_checksum == expected_checksum)
    {
        cout << "[" << name << "] >> Checksum OK! Sende ACK." << endl;
        send_byte_raw(ACK_BYTE);
        global_stats.bytes_received++;
        return received_byte;
    }
    else
    {
        cout << "[" << name << "] XX Checksum FEHLER! Sende NACK." << endl;
        send_byte_raw(NACK_BYTE);
        global_stats.checksum_errors++;
        return 0xFF; // Signalisiert Fehler
    }
}

void B15Simulator::run_sender_mode()
{
    cout << "\n[" << name << "] INTERAKTIVER MODUS (mit ARQ)" << endl;
    cout << "[" << name << "] Gib Nachrichten ein (eine pro Zeile):" << endl;
    cout << "[" << name << "] Jede Zeile wird komplett gesendet + EOT" << endl;

    string line;
    while (getline(cin, line))
    {
        cout << "[" << name << "] Sende Nachricht: \"" << line << "\"" << endl;

        // Sende alle Zeichen der Zeile
        for (char c : line)
        {
            if (!send_byte_with_checksum(c))
            {
                cerr << "[" << name << "] Uebertragung abgebrochen!" << endl;
                global_stats.print();
                return;
            }
        }

        // Sende Newline
        if (!send_byte_with_checksum('\n'))
        {
            cerr << "[" << name << "] Uebertragung abgebrochen!" << endl;
            global_stats.print();
            return;
        }

        // Sende EOT (End of Transmission)
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

void B15Simulator::run_receiver_mode()
{
    cout << "\n[" << name << "]  EMPFANGSMODUS (mit ARQ)" << endl;
    cout << "[" << name << "] Warte auf Nachrichten (bis EOT)..." << endl;

    string received_message = "";

    while (true)
    {
        uint8_t byte = receive_byte_with_checksum();

        if (byte == 0xFF)
        {
            // Fehler oder Timeout, warte auf Wiederholung
            continue;
        }

        // Prüfe auf EOT (End of Transmission)
        if (byte == EOT_BYTE)
        {
            cout << "[" << name << "] EOT empfangen! Nachricht komplett." << endl;
            cout << "[" << name << "] EMPFANGENE NACHRICHT:" << endl;
            cout << ">>> " << received_message << " <<<" << endl;

            // Reset für nächste Nachricht
            received_message = "";
            continue;
        }

        // Sammle Zeichen
        received_message += (char)byte;
        cout << "[" << name << "] Zeichen empfangen: '" << (char)byte
             << "' (Message bisher: \"" << received_message << "\")" << endl;
    }
}

// ==================== FULL-DUPLEX MODE ====================
// NOTE: This implementation uses a mutex to serialize cable access.
// True simultaneous full-duplex would require independent channels,
// but with only 4 bits per board, we time-multiplex the channel.

struct FullDuplexThreadData
{
    B15Simulator *sim;
    volatile bool *running;
    pthread_mutex_t *cable_mutex;
};

void *fullduplex_tx_thread(void *arg)
{
    FullDuplexThreadData *data = (FullDuplexThreadData *)arg;
    B15Simulator *sim = data->sim;
    pthread_mutex_t *mutex = data->cable_mutex;

    cout << "\n[" << sim->name << " TX] SENDEMODUS (Full-Duplex)" << endl;
    cout << "[" << sim->name << " TX] Gib Nachrichten ein (Ctrl+C zum Beenden):" << endl;

    string message;
    while (*(data->running))
    {
        cout << "[" << sim->name << " TX] Eingabe: ";
        if (!getline(cin, message))
        {
            break;
        }

        if (message.empty())
        {
            continue;
        }

        // Sende Nachricht Byte für Byte (mit Mutex-Schutz)
        for (size_t i = 0; i < message.length(); ++i)
        {
            uint8_t byte = message[i];
            bool success = false;

            for (int retry = 0; retry < MAX_RETRIES && !success; ++retry)
            {
                if (retry > 0)
                {
                    cout << "[" << sim->name << " TX] Retry " << retry << " fuer Byte '" << (char)byte << "'" << endl;
                }

                pthread_mutex_lock(mutex);
                success = sim->send_byte_with_checksum(byte);
                pthread_mutex_unlock(mutex);

                if (!success)
                {
                    this_thread::sleep_for(chrono::milliseconds(10));
                }
            }

            if (!success)
            {
                cerr << "[" << sim->name << " TX] Uebertragung fehlgeschlagen!" << endl;
                break;
            }
        }

        // Sende EOT
        bool eot_sent = false;
        for (int retry = 0; retry < MAX_RETRIES && !eot_sent; ++retry)
        {
            pthread_mutex_lock(mutex);
            eot_sent = sim->send_byte_with_checksum(EOT_BYTE);
            pthread_mutex_unlock(mutex);

            if (!eot_sent)
            {
                this_thread::sleep_for(chrono::milliseconds(10));
            }
        }

        if (eot_sent)
        {
            cout << "[" << sim->name << " TX] >>> Nachricht gesendet! <<<\n"
                 << endl;
        }
    }

    return nullptr;
}

void *fullduplex_rx_thread(void *arg)
{
    FullDuplexThreadData *data = (FullDuplexThreadData *)arg;
    B15Simulator *sim = data->sim;
    pthread_mutex_t *mutex = data->cable_mutex;

    // Open output file for this board
    string filename = "received_" + sim->name.substr(sim->name.find(' ') + 1) + ".txt";
    ofstream outfile(filename, ios::app); // Append mode

    cout << "[" << sim->name << " RX] EMPFANGSMODUS (Full-Duplex)" << endl;
    cout << "[" << sim->name << " RX] Schreibe empfangene Nachrichten in: " << filename << endl;
    cout << "[" << sim->name << " RX] Warte auf Nachrichten..." << endl;

    string received_message = "";

    while (*(data->running))
    {
        pthread_mutex_lock(mutex);
        uint8_t byte = sim->receive_byte_with_checksum();
        pthread_mutex_unlock(mutex);

        if (byte == 0xFF)
        {
            // Fehler oder Timeout
            this_thread::sleep_for(chrono::milliseconds(10));
            continue;
        }

        if (byte == EOT_BYTE)
        {
            cout << "[" << sim->name << " RX] >>> NACHRICHT EMPFANGEN: \""
                 << received_message << "\" <<<" << endl;

            // Write to file
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

    return nullptr;
}

void B15Simulator::run_fullduplex_mode()
{
    cout << "\n========================================" << endl;
    cout << "  FULL-DUPLEX MODE - " << name << endl;
    cout << "========================================" << endl;
    cout << "Beide Boards koennen gleichzeitig senden und empfangen!" << endl;
    cout << "Gib Nachrichten ein, um sie zu senden." << endl;
    cout << "Empfangene Nachrichten werden automatisch angezeigt." << endl;
    cout << "========================================\n"
         << endl;

    cout << "HINWEIS: Full-Duplex nutzt Mutex-Locks, um Kollisionen" << endl;
    cout << "         zu vermeiden. TX und RX teilen sich das Kabel.\n"
         << endl;

    volatile bool running = true;
    pthread_mutex_t cable_mutex;
    pthread_mutex_init(&cable_mutex, nullptr);

    pthread_t tx_thread, rx_thread;

    FullDuplexThreadData tx_data = {this, &running, &cable_mutex};
    FullDuplexThreadData rx_data = {this, &running, &cable_mutex};

    // Starte TX Thread (Sender)
    pthread_create(&tx_thread, nullptr, fullduplex_tx_thread, &tx_data);

    // Starte RX Thread (Empfänger)
    pthread_create(&rx_thread, nullptr, fullduplex_rx_thread, &rx_data);

    // Warte auf beide Threads
    pthread_join(tx_thread, nullptr);

    running = false;
    pthread_join(rx_thread, nullptr);

    pthread_mutex_destroy(&cable_mutex);

    cout << "\n[" << name << "] Full-Duplex Mode beendet." << endl;
    global_stats.print();
}
