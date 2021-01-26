#include "Crc16.h"

Crc16Class::Crc16Class()
{
    // Constructor
}

unsigned short Crc16Class::ComputeChecksum(uint8_t *data, uint32_t start, uint32_t length)
{
    uint16_t fcs = 0xffff;
    for (int i = start; i < (start + length); i++)
    {
        uint8_t index = (fcs ^ data[i]) & 0xff;
        fcs = (uint16_t)((fcs >> 8) ^ table[index]);
    }
    fcs ^= 0xffff;
    return fcs;
}
