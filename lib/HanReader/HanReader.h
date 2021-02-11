#ifndef _HANREADER_h
#define _HANREADER_h

#ifdef ARDUINO
#include <Arduino.h>
#include "DlmsReader.h"


class HanReader
{
public:
	HanReader(Stream *hanPort);
	void saveData();
	bool read();
	void print();
	void clear();
private:
	Stream *han;
	
	uint8_t buffer[DLMS_READER_BUFFER_SIZE] = {0};
	StaticJsonDocument<DLMS_READER_BUFFER_SIZE*2> jsonData;
	uint32_t bytesRead = 0;
	DlmsReader reader;

	uint badData[10] = {0};
	double goodData = 0;
	double totalData = 0;
	double invalidLen = 0;
};

#endif //ARDUINO

#endif