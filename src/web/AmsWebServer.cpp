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
	server.on("/raw.dat", HTTP_GET, std::bind(&AmsWebServer::rawData, this));
	server.on("/update", HTTP_GET, std::bind(&AmsWebServer::updateHtml, this));
	server.on("/update", HTTP_POST, 
		std::bind(&AmsWebServer::handleUpdate, this),
		std::bind(&AmsWebServer::handleUpdateUpload, this));
	server.begin(); // Web server start
}

void AmsWebServer::loop() {
	server.handleClient();
}

void AmsWebServer::setDataJson(String str){
	dataJsonStr = str;
}

void AmsWebServer::setRawData(String str){
	rawDataStr = str;
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

void AmsWebServer::rawData() {
	server.sendHeader("Connection", "close");
	server.sendHeader("Access-Control-Allow-Origin","*");
	server.setContentLength(rawDataStr.length());
	server.send(200, "text/plain", rawDataStr);
}

void AmsWebServer::updateHtml() {
	String html = F("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
		"<meta name='viewport' content='width=device-width, initial-scale=1'>"
		"<title>OTA Update</title>"
		"<style>"
		"body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }"
		".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
		"h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }"
		".upload-form { margin: 20px 0; }"
		"input[type='file'] { display: block; margin: 10px 0; padding: 10px; border: 2px dashed #4CAF50; border-radius: 4px; width: 100%; box-sizing: border-box; }"
		"input[type='submit'] { background: #4CAF50; color: white; padding: 12px 24px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; }"
		"input[type='submit']:hover { background: #45a049; }"
		".info { background: #e7f3ff; padding: 15px; border-left: 4px solid #2196F3; margin: 20px 0; }"
		".warning { background: #fff3cd; padding: 15px; border-left: 4px solid #ffc107; margin: 20px 0; }"
		".progress { display: none; margin: 20px 0; }"
		".progress-bar { width: 100%; height: 30px; background: #f0f0f0; border-radius: 4px; overflow: hidden; }"
		".progress-fill { height: 100%; background: #4CAF50; width: 0%; transition: width 0.3s; text-align: center; line-height: 30px; color: white; }"
		"a { color: #4CAF50; text-decoration: none; }"
		"a:hover { text-decoration: underline; }"
		"</style>"
		"</head><body>"
		"<div class='container'>"
		"<h1>Firmware Update (OTA)</h1>"
		"<div class='info'><strong>Info:</strong> Upload a new firmware binary (.bin file) to update the device over-the-air.</div>"
		"<div class='warning'><strong>Warning:</strong> Do not disconnect power during the update process. The device will restart automatically after a successful update.</div>"
		"<form method='POST' action='/update' enctype='multipart/form-data' class='upload-form' id='upload_form'>"
		"<label for='file'><strong>Select firmware file:</strong></label>"
		"<input type='file' name='update' id='file' accept='.bin' required>"
		"<input type='submit' value='Upload Firmware'>"
		"</form>"
		"<div class='progress' id='progress'>"
		"<p>Uploading... <span id='percent'>0%</span></p>"
		"<div class='progress-bar'><div class='progress-fill' id='bar'></div></div>"
		"</div>"
		"<div style='margin-top: 30px; text-align: center;'><a href='/'>← Back to Main Page</a></div>"
		"</div>"
		"<script>"
		"document.getElementById('upload_form').addEventListener('submit', function(e) {"
		"  e.preventDefault();"
		"  var form = e.target;"
		"  var data = new FormData(form);"
		"  var xhr = new XMLHttpRequest();"
		"  xhr.open('POST', '/update', true);"
		"  document.getElementById('progress').style.display = 'block';"
		"  xhr.upload.addEventListener('progress', function(e) {"
		"    if (e.lengthComputable) {"
		"      var percent = Math.round((e.loaded / e.total) * 100);"
		"      document.getElementById('percent').textContent = percent + '%';"
		"      document.getElementById('bar').style.width = percent + '%';"
		"      document.getElementById('bar').textContent = percent + '%';"
		"    }"
		"  });"
		"  xhr.addEventListener('load', function() {"
		"    if (xhr.status === 200) {"
		"      alert('Upload successful! Device will restart in a few seconds.');"
		"      setTimeout(function() { window.location.href = '/'; }, 5000);"
		"    } else {"
		"      alert('Upload failed: ' + xhr.responseText);"
		"      document.getElementById('progress').style.display = 'none';"
		"    }"
		"  });"
		"  xhr.addEventListener('error', function() {"
		"    alert('Upload failed due to network error.');"
		"    document.getElementById('progress').style.display = 'none';"
		"  });"
		"  xhr.send(data);"
		"});"
		"</script>"
		"</body></html>");
	server.send(200, "text/html", html);
}

void AmsWebServer::handleUpdate() {
	server.sendHeader("Connection", "close");
	server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
	ESP.restart();
}

void AmsWebServer::handleUpdateUpload() {
	HTTPUpload& upload = server.upload();
	if (upload.status == UPLOAD_FILE_START) {
		Serial.setDebugOutput(true);
		Serial.printf("Update: %s\n", upload.filename.c_str());
		uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
		if (!Update.begin(maxSketchSpace)) {
			Update.printError(Serial);
		}
	} else if (upload.status == UPLOAD_FILE_WRITE) {
		if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
			Update.printError(Serial);
		}
	} else if (upload.status == UPLOAD_FILE_END) {
		if (Update.end(true)) {
			Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
		} else {
			Update.printError(Serial);
		}
		Serial.setDebugOutput(false);
	}
	yield();
}