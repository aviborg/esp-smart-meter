#include "DlmsReader.h"

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

bool DlmsReader::ParseData(uint8_t *buffer, uint32_t length)
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

bool DlmsReader::ParseAXDR(uint8_t *buffer, uint32_t length)
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

    uint32_t position = 1; // Already checked position 0
    uint32_t startPos = 0;
    char tempStr[60] = {0};
    uint32_t tempData;
    JsonObject jsonObject;
    // JsonObject jsonObject = jsonData->createNestedObject("apdu");
    // JsonObject jsonPayload;
    // Check for a second start flag and then just skip
    if (buffer[position] == DLMS_READER_AXDR_STARTSTOP_FLAG)
        ++position;
    startPos = position; // Save start position needed for CRC later
    jsonObject = jsonData->createNestedObject("header");
    jsonObject["encoding"] = "A-XDR";
    jsonObject["frameformat"] = GetFrameFormatLength(position, buffer);
    jsonObject["segmentation"] = segmentation;
    jsonObject["datalength"] = dataLength;
    if (dataLength > length || dataLength > DLMS_READER_BUFFER_SIZE) {
        sprintf(tempStr, "Received length %d not equal to message length %d", length, dataLength);
        (*jsonData)["lengtherror"] = tempStr;
        return false;
    }
    jsonObject["client"] = GetAddress(position, buffer); // Get client address
    jsonObject["server"] = GetAddress(position, buffer); // Get server address
    jsonObject["control"] = buffer[position++];          // Get Control field assuming one uint8_t only
    // Check header CRC
    tempData = GetChecksum(position, buffer);
    if (Crc16.ComputeChecksum(buffer, startPos, position - startPos, 0, 0xffff, 0xffff) != tempData) {
        sprintf(tempStr, "Calculated CRC %04x not equal to message CRC %04x", Crc16.ComputeChecksum(buffer, startPos, position - startPos, 0, 0xffff, 0xffff), tempData);
        (*jsonData)["crcerror"] = tempStr;
        return false;
    }
    sprintf(tempStr, "%04x", tempData);
    jsonObject["hcs"] = tempStr;
    position += 2;
    // Check frame CRC
    tempData = GetChecksum(dataLength + startPos - 2, buffer);
    if (Crc16.ComputeChecksum(buffer, startPos, dataLength - startPos - 1, 0, 0xffff, 0xffff) != tempData) {
                sprintf(tempStr, "Calculated CRC %04x not equal to message CRC %04x", Crc16.ComputeChecksum(buffer, startPos, dataLength - startPos - 1, 0, 0xffff, 0xffff), tempData);
        (*jsonData)["crcerror"] = tempStr;
        return false;
    }
    sprintf(tempStr, "%04x", tempData);
    jsonObject["fcs"] = tempStr;
    // LLC
    tempData = 0;
    for (startPos = position; position < startPos + 3; ++position)
        tempData = tempData << 8 | buffer[position];
    sprintf(tempStr, "%06x", tempData);
    jsonObject["llc"] = tempStr;
    //  Data (APDU)
    jsonObject = jsonData->createNestedObject("apdu");
    jsonObject["tag"] = buffer[position++];
    tempData = 0;
    for (startPos = position; position < startPos + 4; ++position)
        tempData = tempData << 8 | buffer[position];
    sprintf(tempStr, "%08x", tempData);
    jsonObject["liiap"] = tempStr;
    // Date-Time, only length 0 and 0x0C is accepted
    tempData = 12;
    while ((buffer[position] != 0) && (buffer[position] != 0x0C) && --tempData)
        ++position;
    jsonObject["datetime"] = GetOctetString(position, buffer, buffer[position] + 1);

    // Payload
    jsonObject = jsonData->createNestedObject("payload"); // Create unique payload string
    return GetPayloadAXDR(jsonObject, position, buffer);
}

bool DlmsReader::ParseASCII(uint8_t *buffer, uint32_t length)
{
    uint32_t index;
    char row[256];
    const char replaceC[] = "\\";
    const char rowSep[] = "\r\n";
    const char eleSep[] = "()*";
    char *token, *element, *save_ptr;
    uint16_t crcCalc, crcMess;
    JsonObject jsonObject;
    // JsonObject jsonObject;
    JsonArray jsonArray;
    char *payload = reinterpret_cast<char *>(buffer);

    // Find end of message and compute checksum
    index = strcspn(payload, "!");
    if (index >= length)
    {
        (*jsonData)["indexerror"] = "No ! was found in message";
        return false;
    }
    crcCalc = Crc16.ComputeChecksum(buffer, 0, index + 1, 1, 0, 0);
    crcMess = static_cast<uint16_t>(strtol(payload + index + 1, NULL, 16));
    if (crcMess != crcCalc)
    {
        sprintf(row, "Calculated CRC: %04X not equal to message CRC: %04X", crcCalc, crcMess);
        (*jsonData)["crcerror"] = row;
        return false;
    }

    // Replace escape characters and other
    for (element = strpbrk(payload, replaceC); element != NULL; element = strpbrk(element + 1, replaceC))
        *element = '_';

    // Get each row and parse
    payload = payload + 1;                        // Skip leading /
    strtok(payload, "!");                         // Replaces ! to NULL
    token = strtok_r(payload, rowSep, &save_ptr); // token will now hold first row
    jsonObject = jsonData->createNestedObject("header");
    jsonObject["id"] = token; // Get meter id
    jsonObject["encoding"] = "ASCII";
    sprintf(row, "%04X", crcCalc);
    jsonObject["crc"] = row;
    jsonObject = jsonData->createNestedObject("payload");
    jsonObject = jsonObject.createNestedObject(token);
    token = strtok_r(NULL, rowSep, &save_ptr);
    while (token != NULL)
    {
        strcpy(row, token);
        element = strtok(row, eleSep);
        JsonArray array = jsonObject.createNestedArray(element); // First element is the OBIS code
        // Get data elements
        for (element = strtok(NULL, eleSep); element != NULL; element = strtok(NULL, eleSep))
        {
            if (strpbrk(element, "-:kWhkvarhVAms") == NULL)
                array.add(atof(element));
            else
                array.add(element);
        }
        token = strtok_r(NULL, rowSep, &save_ptr);
    }
    return true;
}

// template <typename T>
bool DlmsReader::GetPayloadAXDR(JsonObject &jsonObject, uint32_t &position, uint8_t *buffer)
{
    // See IEC 62056-6-2 Table 2 for definitions
    uint32_t n = 0;
    switch (buffer[position++])
    {
    case 0: // null-data
        // Do nothing?
        break;
    case 1: // array
        // Fall through to next
    case 2: // structure
    {
        JsonObject nestedObject;
        JsonArray nestedArray;
        n = buffer[position + 1];  // Peek next and get type
        if ( n == 1 || n == 2 || n == 19)
            for (n = buffer[position++]; n; --n) {
                if (!GetPayloadAXDR(jsonObject, position, buffer))
                    return false;
            }
        else
        {
            if (n == 10)
            {
                n = buffer[position++] - 1; // length minus first object
                position += 2;
                nestedArray = jsonObject.createNestedArray(GetVisibleString(position, buffer, buffer[position - 1]));
            }
            else if (n == 9)
            {
                n = buffer[position++] - 1; // length minus first object
                position += 2;
                nestedArray = jsonObject.createNestedArray(GetOctetString(position, buffer, buffer[position - 1]));
            }
            else 
            {
                n = buffer[position++];
                nestedArray = jsonObject.createNestedArray("data");
            }
            for (; n; --n)
                if (!GetPayloadAXDR(nestedArray, position, buffer))
                    return false;
        }
        break;
    }
    case 19: // compact-array
        // TODO
        return false;
        break;
    default:
        // Something went wrong
        return false;
        break;
    }
    return true;
}

bool DlmsReader::GetPayloadAXDR(JsonArray &jsonArray, uint32_t &position, uint8_t *buffer)
{
    // See IEC 62056-6-2 Table 2 for definitions
    uint32_t n = 0;
    JsonArray nestedJson;
    switch (buffer[position++])
    {
    case 0: // null-data
        // Do nothing?
        break;
    case 1: // array
        // Fall through to next
    case 2: // structure
        nestedJson = jsonArray.createNestedArray();
        for (n = buffer[position++]; n; --n)
        {
            if (!GetPayloadAXDR(nestedJson, position, buffer))
            {
                return false;
            }
        }
        break;
    case 3: // boolean
        jsonArray.add(GetData<bool>(position, buffer));
        break;
    case 4: // bit-string
        jsonArray.add(GetData<uint8_t>(position, buffer));
        break;
    case 5: // int32
        jsonArray.add(GetData<int32_t>(position, buffer));
        break;
    case 6: // uint32
        jsonArray.add(GetData<uint32_t>(position, buffer));
        break;
    case 9: // octet-string
        n = buffer[position++];
        jsonArray.add(GetOctetString(position, buffer, n));
        break;
    case 10: // visible-string
        n = buffer[position++];
        jsonArray.add(GetVisibleString(position, buffer, n));
        break;
    case 12: // utf8-String
        // TODO
        return false;
        break;
    case 13: // bcd int8
        // TODO, this is not correct
        jsonArray.add(GetData<int8_t>(position, buffer));
        break;
    case 15: // int8
        jsonArray.add(GetData<int8_t>(position, buffer));
        break;
    case 16: // int16
        jsonArray.add(GetData<int16_t>(position, buffer));
        break;
    case 17: // uint8
        jsonArray.add(GetData<uint8_t>(position, buffer));
        break;
    case 18: // uint16
        jsonArray.add(GetData<uint16_t>(position, buffer));
        break;
    case 19: // compact-array
        // TODO
        return false;
        break;
    case 20: // int64
        jsonArray.add(GetData<int64_t>(position, buffer));
        break;
    case 21: // uint64
        jsonArray.add(GetData<uint64_t>(position, buffer));
        break;
    case 22: // enum8, see table 4
        jsonArray.add(GetEnum(position, buffer));
        break;
    case 23: // float32
        jsonArray.add(GetData<float>(position, buffer));
        break;
    case 24: // float64
        jsonArray.add(GetData<double>(position, buffer));
        break;
    case 25: // date-time, octet-string12
        n = 12;
        jsonArray.add(GetOctetString(position, buffer, n));
        break;
    case 26: // date, octet-string5
        n = 5;
        jsonArray.add(GetOctetString(position, buffer, n));
        break;
    case 27: // time, octet-string4
        n = 4;
        jsonArray.add(GetOctetString(position, buffer, n));
        break;
    default:
        // Something went wrong
        return false;
        break;
    }
    return true;
}

uint32_t DlmsReader::GetAddress(uint32_t &position, uint8_t *buffer)
{
    unsigned long address = buffer[position++];
    unsigned char n = 0; // Limit to 4 uint8_t size to prevent endless looping and overflow
    while ((buffer[position - 1] & 0x01) == 0 && n++ < 3)
    {
        address = (address << 8) | buffer[position++];
    }
    return address;
}

template <typename T>
T DlmsReader::GetData(uint32_t &position, uint8_t *buffer)
{
    uint64_t tempData = 0;
    uint64_t *tempPtr = NULL;
    tempPtr = &tempData;
    T value;
    for (uint32_t n = sizeof(T); n; --n)
        tempData = (tempData << 8) | buffer[position++];
    // value = *(T *)&tempData;  // warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
    value = *reinterpret_cast<T *>(tempPtr);
    return value;
}

String DlmsReader::GetOctetString(uint32_t &position, uint8_t *buffer, uint32_t len)
{
    String dataStr = "";
    char tempStr[4] = {0};
    for (uint32_t n = len; n; --n)
    {
        snprintf(tempStr, 4, "%02X", buffer[position++]);
        dataStr += tempStr;
    }
    return dataStr;
}

String DlmsReader::GetVisibleString(uint32_t &position, uint8_t *buffer, uint32_t len)
{
    String dataStr = "";
    char tempStr[2] = {0};
    for (uint32_t n = len; n; --n)
    {
        snprintf(tempStr, 2, "%c", buffer[position++]);
        dataStr += tempStr;
    }
    return dataStr;
}

const char *DlmsReader::GetEnum(uint32_t &position, uint8_t *buffer)
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

uint32_t DlmsReader::GetFrameFormatLength(uint32_t &position, uint8_t *buffer)
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

uint16_t DlmsReader::GetChecksum(uint32_t checksumPosition, uint8_t *buffer)
{
    return (uint16_t)(buffer[checksumPosition + 1] << 8 |
                      buffer[checksumPosition]);
}