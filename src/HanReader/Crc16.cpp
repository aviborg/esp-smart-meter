#include "Crc16.h"

Crc16Class::Crc16Class()
{
    // Constructor
}

unsigned short Crc16Class::ComputeChecksum(byte *data, int start, int length)
{
    ushort fcs = 0xffff;
    for (int i = start; i < (start + length); i++)
    {
        byte index = (fcs ^ data[i]) & 0xff;
        fcs = (ushort)((fcs >> 8) ^ table[index]);
    }
    fcs ^= 0xffff;
    return fcs;
}
