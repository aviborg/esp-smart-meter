#include <ArduinoJson.h>
#include <LittleFS.h>
#include "AmsWebServer.h"

#include "root/index_html.h"
#include "root/styles_css.h"
#include "root/readdata_js.h"

AmsWebServer::AmsWebServer() {
}

AmsWebServer::~AmsWebServer() {
}

void AmsWebServer::setup() {
	server.on("/", std::bind(&AmsWebServer::indexHtml, this));
	server.on("/styles.css", HTTP_GET, std::bind(&AmsWebServer::stylesCss, this));
	server.on("/readdata.js", HTTP_GET, std::bind(&AmsWebServer::readdataJs, this)); 
	server.on("/data.json", HTTP_GET, std::bind(&AmsWebServer::dataJson, this));
	server.on("/log.txt", HTTP_GET, std::bind(&AmsWebServer::logTxt, this));
	server.begin(); // Web server start
}

void AmsWebServer::loop() {
	server.handleClient();
}

void AmsWebServer::setDataJson(String str){
	dataJsonStr = str;
}

void AmsWebServer::indexHtml() {
	String html = String((const __FlashStringHelper*) INDEX_HTML);
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.setContentLength(html.length());
	server.send(200, "text/html", html);
}

void AmsWebServer::stylesCss() {
	String css = String((const __FlashStringHelper*) STYLES_CSS);
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.setContentLength(css.length());
	server.send(200, "text/css", css);
}

void AmsWebServer::readdataJs() {
	String js = String((const __FlashStringHelper*) READDATA_JS);
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.setContentLength(js.length());
	server.send(200, "application/javascript", js);
}

void AmsWebServer::dataJson() {
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Access-Control-Allow-Origin", "*");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.setContentLength(dataJsonStr.length());
	server.send(200, "application/json", dataJsonStr);
}

void AmsWebServer::logTxt() {
	File dataFile = LittleFS.open("/log.txt", "r");
	String txtStr = "no data";
	if (dataFile.isFile()) txtStr = dataFile.readString();
	server.sendHeader("Connection", "close");
	server.sendHeader("Access-Control-Allow-Origin","*");
	server.setContentLength(txtStr.length());
	server.send(200, "text/plain", txtStr);
	dataFile.close();
}