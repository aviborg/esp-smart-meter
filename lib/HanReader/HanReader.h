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
	byte buffer[DLMS_READER_BUFFER_SIZE] = {0};
	StaticJsonDocument<8192> jsonData;
	uint bytesRead = 0;
	DlmsReader reader;

	uint badData[10] = {0};
	double goodData = 0;
	double totalData = 0;
	double invalidLen = 0;
};

#endif //ARDUINO

#endif