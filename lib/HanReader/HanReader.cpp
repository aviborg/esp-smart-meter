#include "HanReader.h"

#ifdef ARDUINO

#include <LittleFS.h>
#include <ESP8266WiFi.h>

HanReader::HanReader(Stream *hanPort)
{
	han = hanPort;
	bytesRead = 0;
}

String HanReader::getHex(){
	String str("");
	if(bytesRead > 0){
		for (uint32_t n = 0; n < bytesRead; ++n){
			str += "0x";
			if(buffer[n] < 16) str += "0";
			str += String(buffer[n], HEX);
			if(n + 1 < bytesRead) str += ", ";
			if((n % 16) == 15) str += '\n';
		}
	}
	return str;
}

String HanReader::parseData()
{
	String dataJsonStr;
	jsonData.clear();
	jsonData["elapsedtime"] = millis();
	jsonData["rssi"] = WiFi.RSSI();
	jsonData["mac"] = WiFi.macAddress();
	jsonData["localip"] = WiFi.localIP().toString();
	jsonData["ssid"] = WiFi.SSID();
	jsonData["hostname"] = WiFi.getHostname();
	if (bytesRead > 0)
	{
		reader.setJson(&jsonData);
		if (!reader.ParseData(buffer, bytesRead))
		{
			File dataFile;
			dataFile = LittleFS.open("/log.txt", "w");
			dataFile.printf("--- Start %lu ---", millis());
			for (uint32_t n = 0; n < bytesRead; ++n)
			{
				if ((n % 16) == 0)
					dataFile.println();
				dataFile.printf("%#04x, ", buffer[n]);
			}
			dataFile.println("\n---  End  ---");
			dataFile.close();
		}
	}
	serializeJson(jsonData, dataJsonStr);
	bytesRead = 0;
	return dataJsonStr;
}

bool HanReader::read()
{
	if (bytesRead >= DLMS_READER_BUFFER_SIZE)
		return false;
	bool dataReceived = han->available() > 0;
	while ((han->available() > 0) && (bytesRead < DLMS_READER_BUFFER_SIZE))
	{
		buffer[bytesRead++] = han->read();
	}
	return dataReceived;
}

#endif //ARDUINO