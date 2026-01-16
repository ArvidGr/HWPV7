#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <b15f/b15f.h>

using namespace std;

const uint8_t ACK_BYTE = 0x06;  // ASCII ACK
const uint8_t NACK_BYTE = 0x15; // ASCII NAK
const uint8_t EOT_BYTE = 0x04;  // ASCII EOT (End of Transmission)
const int MAX_RETRIES = 5;

atomic<bool> running(true);
mutex board_mutex; // WICHTIG: Schützt B15F-Zugriffe gegen Threads!

class Stats
{
public:
    atomic<int> bytes_sent{0};
    atomic<int> bytes_received{0};
    atomic<int> retransmissions{0};
    atomic<int> checksum_errors{0};

    void print()
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
};

Stats global_stats;

// CRC CHECKSUM
uint8_t calculate_checksum(uint8_t data)
{
    uint8_t crc = 0xFF;
    crc ^= data;
    for (int i = 0; i < 8; i++)
    {
        if (crc & 0x80)
        {
            crc = (crc << 1) ^ 0x07;
        }
        else
        {
            crc <<= 1;
        }
    }
    return crc;
}

// ERROR INJECTOR
// Hinweis: Beim echten B15F gibt es keine Fehler-Injektion,
// behalten die Klasse für Kompatibilität
class ErrorInjector
{
private:
    int error_rate_percent;
    mutex inject_mutex;

public:
    ErrorInjector(int rate = 0) : error_rate_percent(rate)
    {
        srand(time(NULL));
    }

    void set_error_rate(int rate)
    {
        lock_guard<mutex> lock(inject_mutex);
        error_rate_percent = rate;
        if (rate > 0)
        {
            cout << "[WARNING] Fehler-Injektion nur in Software-Simulation verfuegbar!" << endl;
        }
    }

    uint8_t inject_error(uint8_t data)
    {
        // Bei echter Hardware: keine Fehler-Injektion
        return data;
    }
};

ErrorInjector error_injector(0);

// B15 BOARD COMMUNICATION
class B15Board
{
public:
    string name; // Public für Thread-Zugriff

private:
    B15F &drv;
    bool verbose;

    // Bit-Masken
    static const uint8_t DATA0 = 0x01;
    static const uint8_t DATA1 = 0x02;
    static const uint8_t CLOCK = 0x04;
    static const uint8_t ACK = 0x08;

    // TX (Senden) Zustände
    uint8_t tx_clock_state;
    uint8_t tx_ack_state;
    uint8_t tx_last_received_ack;

    // RX (Empfangen) Zustände
    uint8_t rx_clock_state;
    uint8_t rx_ack_state;
    uint8_t rx_last_received_clock;

    // LOW-LEVEL B15F ACCESS

    // Schreibt auf Bits 0-3 von PORTA
    void write_output(uint8_t data)
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

    // Liest Bits 7-4 von PINA MIT KREUZUNG!
    // Physikalische Kabelkreuzung: Bit7_PINA <- Bit0_other, Bit6_PINA <- Bit1_other, etc.
    uint8_t read_input()
    {
        lock_guard<mutex> lock(board_mutex);
        uint8_t value = drv.getRegister(&PINA);
        uint8_t upper = (value >> 4) & 0x0F; // Lese Bits 7-4

        // Kabelkreuzung rückgängig machen: PINA[7]->Bit[0], PINA[6]->Bit[1], PINA[5]->Bit[2], PINA[4]->Bit[3]
        uint8_t crossed = 0;
        if (upper & 0x01) // PINA bit 4
            crossed |= 0x08; // wird zu Bit 3
        if (upper & 0x02) // PINA bit 5
            crossed |= 0x04; // wird zu Bit 2
        if (upper & 0x04) // PINA bit 6
            crossed |= 0x02; // wird zu Bit 1
        if (upper & 0x08) // PINA bit 7
            crossed |= 0x01; // wird zu Bit 0

        if (verbose)
        {
            cout << "  [" << name << "] Read Bits 7-4 (crossed): " << bitset<4>(crossed) << endl;
        }

        return crossed;
    }

    // PROTOCOL LAYER

    bool send_2bits(uint8_t data)
    {
        data &= 0x03;

        uint8_t output = tx_ack_state;
        if (data & 0x01)
            output |= DATA0;
        if (data & 0x02)
            output |= DATA1;

        tx_clock_state ^= CLOCK;
        output |= tx_clock_state;

        write_output(output); // TX schreibt DATA+CLOCK auf Bits 0-3

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

    uint8_t receive_2bits()
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
                uint8_t output = rx_ack_state | rx_clock_state;

                write_output(output); // RX schreibt ACK auf Bits 0-3

                return data;
            }

            drv.delay_us(100);
        }

        return 0xFF;
    }

    bool send_byte_raw(uint8_t byte)
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

    uint8_t receive_byte_raw()
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

public:
    B15Board(string board_name, bool verb = false)
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

    void set_verbose(bool v)
    {
        verbose = v;
    }

    // HIGH-LEVEL PROTOCOL

    bool send_byte_with_checksum(uint8_t byte)
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

    uint8_t receive_byte_with_checksum()
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

    void run_sender_mode()
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

    void run_receiver_mode()
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
                << endl;

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

    void sender_thread()
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

    void receiver_thread()
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

    void run_fullduplex_mode()
    {
        cout << "\n[" << name << "]  FULL-DUPLEX MODE" << endl;
        cout << "[" << name << "] Beide Richtungen gleichzeitig aktiv!" << endl;
        cout << "[" << name << "] Empfangene Nachrichten werden in Datei geschrieben." << endl;
        cout << "[" << name << "] HINWEIS: Mutex-Locks schuetzen B15F-Zugriffe!\n"
             << endl;

        thread sender([this]()
                      { this->sender_thread(); });
        thread receiver([this]()
                        { this->receiver_thread(); });

        sender.join();
        running = false;
        receiver.join();

        cout << "\n[" << name << "] Full-Duplex Mode beendet." << endl;
        global_stats.print();
    }
};

// main
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