#ifndef B15SIMULATOR_H
#define B15SIMULATOR_H

#include "patch_cable.h"
#include <string>
#include <cstdint>

class B15Simulator
{
public:
    std::string name; // Public for thread access

private:
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

    // Private Methoden
    void write_output(uint8_t data);
    uint8_t read_input();

    bool send_2bits(uint8_t data);
    uint8_t receive_2bits();
    bool send_byte_raw(uint8_t byte);
    uint8_t receive_byte_raw();

public:
    B15Simulator(bool is_a, bool verb = false);

    bool send_byte_with_checksum(uint8_t byte);
    uint8_t receive_byte_with_checksum();

    void run_sender_mode();
    void run_receiver_mode();
    void run_fullduplex_mode();
};

#endif // B15SIMULATOR_H
