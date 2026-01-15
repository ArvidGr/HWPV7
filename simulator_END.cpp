#include <iostream>
#include <cstdint>
#include <bitset>
#include <fstream>
#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

// Protokoll-Konstanten
const uint8_t ACK_BYTE = 0x06;  // ASCII ACK
const uint8_t NACK_BYTE = 0x15; // ASCII NAK
const uint8_t EOT_BYTE = 0x04;  // ASCII EOT (End of Transmission)
const int MAX_RETRIES = 5;

// Statistik
struct Stats
{
    int bytes_sent = 0;
    int bytes_received = 0;
    int retransmissions = 0;
    int checksum_errors = 0;

    void print()
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
};

Stats global_stats;

// CRC8 Checksumme (einfache Variante)
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

// Simulierte Fehler-Injektion
class ErrorInjector
{
private:
    int error_rate_percent; // 0-100

public:
    ErrorInjector(int rate = 0) : error_rate_percent(rate)
    {
        srand(time(NULL));
    }

    void set_error_rate(int rate)
    {
        error_rate_percent = rate;
        cout << "[ERROR-INJECTOR] Fehlerrate gesetzt auf " << rate << "%" << endl;
    }

    uint8_t inject_error(uint8_t data)
    {
        if (error_rate_percent > 0 && (rand() % 100) < error_rate_percent)
        {
            int bit_to_flip = rand() % 8;
            uint8_t corrupted = data ^ (1 << bit_to_flip);
            cout << "  [ERROR!] Bit " << bit_to_flip << " geflippt: "
                 << bitset<8>(data) << " -> " << bitset<8>(corrupted) << endl;
            return corrupted;
        }
        return data;
    }
};

ErrorInjector error_injector(0);

// Simuliertes Patchkabel als Datei
class PatchCableFile
{
private:
    string filename;

public:
    PatchCableFile(const string &fname = "patchcable.bin") : filename(fname)
    {
        ifstream check(filename);
        if (!check.good())
        {
            ofstream file(filename, ios::binary);
            uint8_t zero = 0;
            file.write((char *)&zero, 1);
            file.close();
            cout << "[CABLE] Patchkabel-Datei erstellt: " << filename << endl;
        }
        else
        {
            cout << "[CABLE] Patchkabel-Datei gefunden: " << filename << endl;
            check.close();
        }
    }

    ~PatchCableFile()
    {
        remove(filename.c_str());
    }

    void write(uint8_t value)
    {
        ofstream file(filename, ios::binary | ios::in | ios::out);
        file.seekp(0);
        file.write((char *)&value, 1);
        file.close();
    }

    uint8_t read()
    {
        ifstream file(filename, ios::binary);
        uint8_t value = 0;
        if (file.is_open())
        {
            file.read((char *)&value, 1);
            file.close();
        }
        return value;
    }

    void write_bits_a(uint8_t data)
    {
        uint8_t current = read();
        uint8_t new_val = (current & 0xF0) | (data & 0x0F);
        write(new_val);
    }

    void write_bits_b(uint8_t data)
    {
        uint8_t current = read();
        uint8_t new_val = (current & 0x0F) | ((data & 0x0F) << 4);
        write(new_val);
    }

    uint8_t read_bits_a()
    {
        return (read() >> 4) & 0x0F;
    }

    uint8_t read_bits_b()
    {
        return read() & 0x0F;
    }
};

class B15Simulator
{
private:
    string name;
    bool is_board_a;
    PatchCableFile cable;
    bool verbose;

    // Bit-Masken
    static const uint8_t DATA0 = 0x01;
    static const uint8_t DATA1 = 0x02;
    static const uint8_t CLOCK = 0x04;
    static const uint8_t ACK = 0x08;

    uint8_t current_clock_state;
    uint8_t last_received_ack;
    uint8_t last_received_clock;
    uint8_t current_ack_state;

    void write_output(uint8_t data)
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

    uint8_t read_input()
    {
        if (is_board_a)
        {
            return cable.read_bits_a();
        }
        else
        {
            return cable.read_bits_b();
        }
    }

    bool send_2bits(uint8_t data)
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

    uint8_t receive_2bits()
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
    B15Simulator(bool is_a, bool verb = false) : is_board_a(is_a), verbose(verb)
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

    void set_verbose(bool v) { verbose = v; }

    // Sendet Byte mit Checksumme und ARQ
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

    // Empfängt Byte mit Checksumme und sendet ACK/NACK
    uint8_t receive_byte_with_checksum()
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

    void interactive_mode()
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

    void listen_mode()
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
};

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " <board> <mode> [error_rate]" << endl;
        cout << "  board:      A oder B" << endl;
        cout << "  mode:       send oder receive" << endl;
        cout << "  error_rate: 0-100 (optional, default: 0)" << endl;
        cout << "\nBeispiel:" << endl;
        cout << "  Terminal 1: " << argv[0] << " A send" << endl;
        cout << "  Terminal 2: " << argv[0] << " B receive 20" << endl;
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
        board_sim.interactive_mode();
    }
    else if (mode == "receive")
    {
        board_sim.listen_mode();
    }
    else
    {
        cerr << "Mode muss 'send' oder 'receive' sein!" << endl;
        return 1;
    }

    return 0;
}