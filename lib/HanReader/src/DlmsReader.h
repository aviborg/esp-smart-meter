#ifndef _DLMSREADER_h
#define _DLMSREADER_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Crc16.h"
#define DLMS_READER_BUFFER_SIZE 4096U
#define DLMS_READER_MAX_ADDRESS_SIZE 5
#define DLMS_READER_AXDR_STARTSTOP_FLAG 0x7E
#define DLMS_READER_ASCII_START_FLAG 0x2F
#define DLMS_READER_ASCII_STOP_FLAG 0x21


class DlmsReader
{
  public:
    DlmsReader();
    bool ParseData(byte *buffer, uint length);
    void setJson(JsonDocument *json);
    int GetRawData(byte *buffer, int start, int length);
    
  protected:
    Crc16Class Crc16;
    
  private:
    ulong dataLength;
    byte segmentation;
    ulong control;
    JsonDocument *jsonData;

    bool ParseAXDR(byte *buffer, uint length);
    bool ParseASCII(byte *buffer, uint length);
    void Clear();
    bool GetPayload(JsonArray &jsonArray, uint &position, byte *buffer);
    ulong GetAddress(uint &position, byte* buffer);
    template<typename T> T GetData(uint &position, byte *buffer);
    String GetOctetString(uint &position, byte *buffer, uint len);
    const char *GetEnum(uint &position, byte *buffer);
    ulong GetFrameFormatLength(uint &position, byte* buffer);
    ushort GetChecksum(uint checksumPosition, byte *buffer);
};

#endif
