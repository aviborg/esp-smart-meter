#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "HanReader.h"
#include "web/AmsWebServer.h"
#include "hw/chipSetup.h"

AmsWebServer webServer;
HanReader hanReader(&Serial);

void setup() {
  Serial.begin(115200);

  pinMode(TRIGGER_PIN, INPUT);      
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);
  
  // Mount filesystem
  if (!LittleFS.begin()) {
    LittleFS.end();
    delay(2000);
    ESP.reset();
  }

  // Setup wifi and webserver
  wifiSetup();
  webServer.setup();
  ArduinoOTA.begin();
  webServer.setDataJson(hanReader.parseData());

  // Flush serial buffer
  while(Serial.available()>0) Serial.read(); 
}

void loop()
{
  static uint32_t lastUpdate = 0xFFFFFFFF; // Test wraparound at startup instead of waiting 50 days
  static uint16_t scheduleState = 0;
  static bool dataReceived = false;
  unsigned long now = millis();
  // Reading serial data should be uninterrupted
  // When a serial read is detected other stuff is delayed 61 ms
  // using a timeout not divisible by 1000 (even) and a prime number
  // reduce the risk of having the server working while data is received
  if (hanReader.read()) {
    lastUpdate = now;
    dataReceived = true;
  }
  else {
    if (now - lastUpdate > 61) {
      lastUpdate = now;
      if (dataReceived) {
        digitalWrite(LED_BUILTIN, LOW); // Lit up LED
        webServer.setRawData(hanReader.getHex());
        webServer.setDataJson(hanReader.parseData());
        dataReceived = false;
      }
      else {
        digitalWrite(LED_BUILTIN, HIGH); // Turn off LED
        switch (scheduleState++) {
          case 0:
            resetChipOnTrigger();
            break;
          case 1:
            MDNS.update();
            break;
          case 2:
            webServer.loop();
            break;
          case 3:
            ArduinoOTA.handle();
            break;
          default:
            yield();
            scheduleState = 0;
            break;
        }
      }
    }
  }
}