#ifndef _AMSWEBSERVER_h
#define _AMSWEBSERVER_h

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

class AmsWebServer
{
public:
	AmsWebServer();
	~AmsWebServer();
	void setup();
	void loop();
	void setDataJson(String str);
	void setRawData(String str);
private:
	ESP8266WebServer server;
	void indexHtml();
	void stylesCss();
	void readdataJs();
	void dataJson();
	void logTxt();
	void rawData();
	String rawDataStr;
	String dataJsonStr;
};

#endif
