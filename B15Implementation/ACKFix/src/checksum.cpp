#include "../include/checksum.h"

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
