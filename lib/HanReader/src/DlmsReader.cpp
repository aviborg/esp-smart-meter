#include "DlmsReader.h"
#include <LittleFS.h>

DlmsReader::DlmsReader()
{
    //this->Clear();
}

void DlmsReader::Clear()
{
    this->dataLength = 0;
}

void DlmsReader::setJson(JsonDocument *json)
{
    this->jsonData = json;
}

bool DlmsReader::ParseData(byte *buffer, uint length)
{
    // Data is only useful if there is a header and a datafield
    if (length > 20U)
    {
        switch (buffer[0])
        {
        case DLMS_READER_AXDR_STARTSTOP_FLAG:
            return ParseAXDR(buffer, length);
            break;
        case DLMS_READER_ASCII_START_FLAG:
            return ParseASCII(buffer, length);
            break;
        }
    }
    return false;
}

bool DlmsReader::ParseAXDR(byte *buffer, uint length)
{
    /* A data package should have the following format:
    See ISO/IEC 13239:2002
    HDLC: 
    +-----------------------------------------------------------------------------+
    | Flag | Frame  | Client  | Server  | Control | HCS | LLC | Data | FCS | Flag |
    |      | Format | Address | Address |         |     |     |      |     |      |
    +-----------------------------------------------------------------------------+
    Data (APDU):
    +----------------------------------------------+
    | Tag | Long-invoke-id- | Date-Time  | Payload |
    |     | and-priority    |            |         |
    +----------------------------------------------+
    Payload:
    +---------------------------------------------------+
    | Datatype | Length | Register 1 | ... | Register n |
    +---------------------------------------------------+  */

    uint position = 1; // Already checked position 0
    uint startPos = 0;
    char tempStr[33] = {0};
    uint tempData;
    JsonObject jsonHeader = jsonData->createNestedObject("header");
    JsonObject jsonAPDU = jsonData->createNestedObject("apdu");
    JsonArray jsonPayload;
    // Check for a second start flag and then just skip
    if (buffer[position] == DLMS_READER_AXDR_STARTSTOP_FLAG)
        ++position;
    startPos = position; // Save start position needed for CRC later
    jsonHeader["frameformat"] = GetFrameFormatLength(position, buffer);
    jsonHeader["segmentation"] = segmentation;
    jsonHeader["datalength"] = dataLength;
    if (dataLength > length || dataLength > DLMS_READER_BUFFER_SIZE)
        return false;
    jsonHeader["client"] = GetAddress(position, buffer); // Get client address
    jsonHeader["server"] = GetAddress(position, buffer); // Get server address
    jsonHeader["control"] = buffer[position++];          // Get Control field assuming one byte only
    // Check header CRC
    tempData = GetChecksum(position, buffer);
    if (Crc16.ComputeChecksum(buffer, startPos, position - startPos) != tempData)
        return false;
    sprintf(tempStr, "%#06X", tempData);
    jsonHeader["hcs"] = tempStr;
    sprintf(tempStr, "payload%#04X", tempData);
    jsonPayload = jsonData->createNestedArray(tempStr); // Create unique payload string
    position += 2;
    // Check frame CRC
    tempData = GetChecksum(dataLength + startPos - 2, buffer);
    if (Crc16.ComputeChecksum(buffer, startPos, dataLength - startPos - 1) != tempData)
        return false;
    sprintf(tempStr, "%#06X", tempData);
    jsonHeader["fcs"] = tempStr;
    // LLC
    tempData = 0;
    for (startPos = position; position < startPos + 3; ++position)
        tempData = tempData << 8 | buffer[position];
    sprintf(tempStr, "%#08X", tempData);
    jsonHeader["llc"] = tempStr;
    //  Data (APDU)
    jsonAPDU["tag"] = buffer[position++];
    tempData = 0;
    for (startPos = position; position < startPos + 4; ++position)
        tempData = tempData << 8 | buffer[position];
    sprintf(tempStr, "%#010X", tempData);
    jsonAPDU["liiap"] = tempStr;
    // Date-Time, only length 0 and 0x0C is accepted
    tempData = 12;
    while ((buffer[position] != 0) && (buffer[position] != 0x0C) && --tempData) 
        ++position;
    jsonAPDU["datetime"] = GetOctetString(position, buffer, buffer[position]+1);

    // Payload
    return GetPayload(jsonPayload, position, buffer);
}

bool DlmsReader::ParseASCII(byte *buffer, uint length)
{
    return false;
}

bool DlmsReader::GetPayload(JsonArray &jsonData, uint &position, byte *buffer)
{
    // See IEC 62056-6-2 Table 2 for definitions
    uint n = 0;
    switch (buffer[position++])
    {
    case 0: // null-data
        // Do nothing?
        break;
    case 1: // array
        // Fall through to next
    case 2: // structure
    {
        JsonArray array = jsonData.createNestedArray();
        for (n = buffer[position++]; n; --n)
        {
            if (!GetPayload(array, position, buffer))
            {
                return false;
            }
        }
        break;
    }
    case 3: // boolean
        jsonData.add(GetData<bool>(position, buffer));
        break;
    case 4: // bit-string
        jsonData.add(GetData<uint8_t>(position, buffer));
        break;
    case 5: // int32
        jsonData.add(GetData<int32_t>(position, buffer));
        break;
    case 6: // uint32
        jsonData.add(GetData<uint32_t>(position, buffer));
        break;
    case 9: // octet-string
        n = buffer[position++];
        jsonData.add(GetOctetString(position, buffer, n));
        break;
    case 10: // visible-string
        n = buffer[position++];
        jsonData.add(GetOctetString(position, buffer, n));
        break;
    case 12: // utf8-String
        // TODO
        return false;
        break;
    case 13: // bcd int8
        // TODO, this is not correct
        jsonData.add(GetData<int8_t>(position, buffer));
        break;
    case 15: // int8
        jsonData.add(GetData<int8_t>(position, buffer));
        break;
    case 16: // int16
        jsonData.add(GetData<int16_t>(position, buffer));
        break;
    case 17: // uint8
        jsonData.add(GetData<uint8_t>(position, buffer));
        break;
    case 18: // uint16
        jsonData.add(GetData<uint16_t>(position, buffer));
        break;
    case 19: // compact-array
        // TODO
        return false;
        break;
    case 20: // int64
        jsonData.add(GetData<int64_t>(position, buffer));
        break;
    case 21: // uint64
        jsonData.add(GetData<uint64_t>(position, buffer));
        break;
    case 22: // enum8, see table 4
        jsonData.add(GetEnum(position, buffer));
        break;
    case 23: // float32
        jsonData.add(GetData<float_t>(position, buffer));
        break;
    case 24: // float64
        jsonData.add(GetData<double_t>(position, buffer));
        break;
    case 25: // date-time, octet-string12
        n = 12;
        jsonData.add(GetOctetString(position, buffer, n));
        break;
    case 26: // date, octet-string5
        n = 5;
        jsonData.add(GetOctetString(position, buffer, n));
        break;
    case 27: // time, octet-string4
        n = 4;
        jsonData.add(GetOctetString(position, buffer, n));
        break;
    default:
        // Something went wrong
        return false;
        break;
    }
    return true;
}

ulong DlmsReader::GetAddress(uint &position, byte *buffer)
{
    unsigned long address = buffer[position++];
    unsigned char n = 0; // Limit to 4 byte size to prevent endless looping and overflow
    while ((buffer[position - 1] & 0x01) == 0 && n++ < 3)
    {
        address = (address << 8) | buffer[position++];
    }
    return address;
}

template <typename T>
T DlmsReader::GetData(uint &position, byte *buffer)
{
    uint64_t tempData = 0;
    uint64_t *tempPtr = NULL;
    tempPtr = &tempData;
    T value;
    for (uint n = sizeof(T); n; --n)
        tempData = (tempData << 8) | buffer[position++];
    // value = *(T *)&tempData;  // warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
    value = *reinterpret_cast<T *>(tempPtr);
    return value;
}

String DlmsReader::GetOctetString(uint &position, byte *buffer, uint len)
{
    String dataStr = "";
    char tempStr[4] = {0};
    for (uint n = len; n; --n)
    {
        snprintf(tempStr, 4, "%02X", buffer[position++]);
        dataStr += tempStr;
    }
    return dataStr;
}

const char *DlmsReader::GetEnum(uint &position, byte *buffer)
{
    switch (buffer[position++])
    {
    case 1:
        return "a"; // year
    case 2:
        return "mo";
    case 3:
        return "wk";
    case 4:
        return "d";
    case 5:
        return "h";
    case 6:
        return "min";
    case 7:
        return "s";
    case 8:
        return "°";
    case 9:
        return "°C";
    case 10:
        return "€";
    case 11:
        return "m";
    case 12:
        return "m/s";
    case 13:
        return "m";
    case 14:
        return "m³";
    case 15:
        return "m³/h";
    case 16:
        return "m³/h";
    case 17:
        return "m³/d";
    case 18:
        return "m³/d";
    case 19:
        return "l";
    case 20:
        return "kg";
    case 21:
        return "N";
    case 22:
        return "Nm";
    case 23:
        return "Pa";
    case 24:
        return "bar";
    case 25:
        return "J";
    case 26:
        return "J/h";
    case 27:
        return "W";
    case 28:
        return "VA";
    case 29:
        return "var";
    case 30:
        return "Wh";
    case 31:
        return "VAh";
    case 32:
        return "varh";
    case 33:
        return "A";
    case 34:
        return "C";
    case 35:
        return "V";
    case 36:
        return "V/m";
    case 37:
        return "F";
    case 38:
        return "Ω";
    case 39:
        return "Ωm²/m";
    case 40:
        return "Wb";
    case 41:
        return "T";
    case 42:
        return "A/m";
    case 43:
        return "H";
    case 44:
        return "Hz";
    case 45:
        return "1/(Wh)";
    case 46:
        return "1/(varh)";
    case 47:
        return "1/(VAh)";
    case 48:
        return "V²h";
    case 49:
        return "A²h";
    case 50:
        return "kg/s";
    case 51:
        return "S";
    case 52:
        return "°K";
    case 53:
        return "1/(V²h)";
    case 54:
        return "1/(A²h)";
    case 55:
        return "1/m³";
    case 56:
        return "%";
    case 57:
        return "Ah";
    case 60:
        return "Wh/m³";
    case 61:
        return "J/m³";
    case 62:
        return "Mol%";
    case 63:
        return "g/m³";
    case 64:
        return "Pa s";
    case 65:
        return "J/kg";
    case 70:
        return "dBm";
    case 71:
        return "dbμV";
    case 72:
        return "dB";
    default:
        return "*";
    }
}

ulong DlmsReader::GetFrameFormatLength(uint &position, byte *buffer)
{
    // Check for frame format and frame length. See 4.9 in IEC13239
    // Only frame format 0 to 7 supported
    unsigned long int frameFormat = (buffer[position] & 0x80) >> 7;

    if (buffer[position] <= 0x7F)
    {
        dataLength = buffer[position++] & 0x7F;
    }
    else if (buffer[position] <= 0xF0)
    {
        frameFormat += (buffer[position] & 0x70) >> 4;
        segmentation = (buffer[position] & 0x08) != 0;
        dataLength = (buffer[position++] & 0x07) << 8;
        dataLength += buffer[position++];
    }
    else
        frameFormat = 8;
    return frameFormat;
}

ushort DlmsReader::GetChecksum(uint checksumPosition, byte *buffer)
{
    return (ushort)(buffer[checksumPosition + 1] << 8 |
                    buffer[checksumPosition]);
}
