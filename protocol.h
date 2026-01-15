#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

// Protokoll-Konstanten
const uint8_t ACK_BYTE = 0x06;  // ASCII ACK
const uint8_t NACK_BYTE = 0x15; // ASCII NAK
const uint8_t EOT_BYTE = 0x04;  // ASCII EOT (End of Transmission)
const int MAX_RETRIES = 5;

#endif // PROTOCOL_H
