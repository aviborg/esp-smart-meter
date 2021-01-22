#include "HanReader.h"
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
			// Something went wrong, dump buffer to log.txt TODO some rotating log
			dataFile = LittleFS.open("/log.txt", "w");
			dataFile.print("--- Start ---");
			for (uint n = 0; n < bytesRead; ++n) {
				if ((n%16) == 0) dataFile.println();
				dataFile.printf("%#04x, ", buffer[n]);
			}
			dataFile.println("\n---  End  ---");
			dataFile.close();
		}
		bytesRead = 0;
	}
	dataFile = LittleFS.open("/data.json", "w");
	serializeJson(jsonData, dataFile);
	dataFile.close();
}

bool HanReader::read()
{
	bool dataRecieved = han->available() > 0;
	while (han->available() > 0)
		buffer[bytesRead++] = han->read();
	return dataRecieved;
}