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
    bool ParseData(byte *buffer, uint32_t length);
    void setJson(JsonDocument *json); 
    int GetRawData(byte *buffer, int start, int length);
    
  protected:
    Crc16Class Crc16;
    
  private:
    uint32_t dataLength;
    byte segmentation;
    uint32_t control;
    JsonDocument *jsonData;

    bool ParseAXDR(byte *buffer, uint32_t length);
    bool ParseASCII(byte *buffer, uint32_t length);
    void Clear();
    bool GetPayload(JsonArray &jsonArray, uint32_t &position, byte *buffer);
    uint32_t GetAddress(uint32_t &position, byte* buffer);
    template<typename T> T GetData(uint32_t &position, byte *buffer);
    String GetOctetString(uint32_t &position, byte *buffer, uint32_t len);
    const char *GetEnum(uint32_t &position, byte *buffer);
    uint32_t GetFrameFormatLength(uint32_t &position, byte* buffer);
    uint16_t  GetChecksum(uint32_t checksumPosition, byte *buffer);
};

#endif
