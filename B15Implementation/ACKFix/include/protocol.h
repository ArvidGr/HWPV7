#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

// Protocol Constants
const uint8_t ACK_BYTE = 0x06;     // ASCII ACK
const uint8_t NACK_BYTE = 0x15;    // ASCII NAK
const uint8_t EOT_BYTE = 0x04;     // ASCII EOT (End of Transmission)
const uint8_t NO_DATA_BYTE = 0x10; // signals "no data to send"
const int MAX_RETRIES = 5;

// Bit-Masken für die 2-Bit Kommunikation
const uint8_t DATA0 = 0x01;
const uint8_t DATA1 = 0x02;
const uint8_t CLOCK = 0x04;
const uint8_t ACK = 0x08;

#endif // PROTOCOL_H
