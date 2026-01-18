#ifndef B15BOARD_H
#define B15BOARD_H

#include <string>
#include <cstdint>
#include <b15f/b15f.h>

class B15Board
{
public:
    std::string name; // Public für Thread-Zugriff

    B15Board(std::string board_name, bool verb = false);

    void set_verbose(bool v);

    // Operation Modes
    void run_sender_mode();
    void run_receiver_mode();
    void run_fullduplex_mode();

    // High-Level Protocol
    bool send_byte_with_checksum(uint8_t byte);
    uint8_t receive_byte_with_checksum();

private:
    B15F &drv;
    bool verbose;

    // TX (Senden) Zustände
    uint8_t tx_clock_state;
    uint8_t tx_ack_state;
    uint8_t tx_last_received_ack;

    // RX (Empfangen) Zustände
    uint8_t rx_clock_state;
    uint8_t rx_ack_state;
    uint8_t rx_last_received_clock;

    // Cached output state (bits 0-3)
    uint8_t cached_output_state;

    // Low-Level B15F Access
    void write_output(uint8_t data);
    uint8_t read_input();

    // Protocol Layer
    bool send_2bits(uint8_t data);
    uint8_t receive_2bits();
    bool send_byte_raw(uint8_t byte);
    uint8_t receive_byte_raw();

    // Full-Duplex Threads
    void sender_thread();
    void receiver_thread();
};

// Globaler Mutex für B15F-Zugriffe
extern std::mutex board_mutex;

// Globales running Flag für Threads
extern std::atomic<bool> running;

#endif // B15BOARD_H
