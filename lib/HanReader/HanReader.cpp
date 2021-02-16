#include "HanReader.h"

#ifdef ARDUINO

#include <LittleFS.h>
#include <ESP8266WiFi.h>

HanReader::HanReader(Stream *hanPort)
{

	han = hanPort;
	bytesRead = 0;
}

void HanReader::saveData()
{
	File dataFile;
	jsonData.clear();
	jsonData["elapsedtime"] = millis();
	jsonData["rssi"] = WiFi.RSSI();
	jsonData["mac"] = WiFi.macAddress();
	jsonData["localip"] = WiFi.localIP().toString();
	jsonData["ssid"] = WiFi.SSID();
	if (bytesRead > 0)
	{
		reader.setJson(&jsonData);
		if (!reader.ParseData(buffer, bytesRead))
		{
			dataFile = LittleFS.open("/log.txt", "w");
			dataFile.print("--- Start ---");
			for (uint n = 0; n < bytesRead; ++n)
			{
				if ((n % 16) == 0)
					dataFile.println();
				dataFile.printf("%#04x, ", buffer[n]);
			}
			dataFile.println("\n---  End  ---");
			dataFile.close();
		}
	}
	dataFile = LittleFS.open("/data.json", "w");
	serializeJson(jsonData, dataFile);
	dataFile.close();
	bytesRead = 0;
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