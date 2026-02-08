#ifndef _AMSWEBSERVER_h
#define _AMSWEBSERVER_h

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

class UpdateManager; // Forward declaration

class AmsWebServer
{
public:
	AmsWebServer();
	~AmsWebServer();
	void setup();
	void loop();
	void setDataJson(String str);
	void setRawData(String str);
	void setUpdateManager(UpdateManager* mgr);
private:
	ESP8266WebServer server;
	UpdateManager* updateManager;
	void indexHtml();
	void stylesCss();
	void readdataJs();
	void dataJson();
	void logTxt();
	void rawData();
	void versionJson();
	void updateCheck();
	void updateTrigger();
	String rawDataStr;
	String dataJsonStr;
};

#endif
