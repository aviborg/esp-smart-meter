#include "chipSetup.h"
#include <LittleFS.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <ArduinoJson.h>

char hostName[HOSTNAME_MAXLENGTH] = HOSTNAME_DEFAULT;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback()
{
    shouldSaveConfig = true;
}

void tick()
{
    //toggle state
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void wifiSetup()
{
    WiFiManager wifiManager;
    Ticker ticker;
    String htmlString;
    if (LittleFS.exists("/config.json"))
    {
        File configFile = LittleFS.open("/config.json", "r");
        if (configFile)
        {
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);
            configFile.readBytes(buf.get(), size);
            DynamicJsonDocument jsonDoc(1024);
            // Deserialize the JSON document
            DeserializationError error = deserializeJson(jsonDoc, buf.get());
            if (!error)
            {
                strcpy(hostName, jsonDoc["hostName"]);
            }
            configFile.close();
        }
    }

    htmlString =  "<br/>When connected, the electricity meter will be available on the local network on address: <br/> <a href=\"http://";
    htmlString += hostName;
    htmlString += "\">http://";
    htmlString += hostName;
    htmlString += "</a><br/>You may change the hostname to a custom name, (Only numbers and lowercase letters, no spaces). ";
    htmlString += "Note that the hostname may take a up to 60 minutes to get registered on your local network.";
    WiFiManagerParameter hostNameParam("hostName", hostName, hostName, HOSTNAME_MAXLENGTH);
    WiFiManagerParameter htmlParam(htmlString.c_str());
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    ticker.attach(0.6, tick);

    wifiManager.addParameter(&htmlParam);
    wifiManager.addParameter(&hostNameParam);
    WiFi.hostname(hostName);
    if (!wifiManager.autoConnect())
    {
        ESP.reset();
        delay(5000);
    }

    strcpy(hostName, hostNameParam.getValue());

    if (shouldSaveConfig)
    {
        DynamicJsonDocument jsonDoc(1024);
        jsonDoc["hostName"] = hostName;
        File configFile = LittleFS.open("/config.json", "w");
        serializeJson(jsonDoc, configFile);
        configFile.close();
    }

    ticker.detach();

    MDNS.begin(hostName);
    MDNS.addService("http", "tcp", HTTP_PORT);
    digitalWrite(LED_BUILTIN, HIGH);
}

void resetChipOnTrigger()
{
    static unsigned long resetCounter = 0;
    if (digitalRead(TRIGGER_PIN) == LOW)
    {
        digitalWrite(LED_BUILTIN, LOW); // Lit up LED when button is pressed
        if (millis() - resetCounter > RESET_TIMEOUT)
        {
            WiFiManager wm;
            wm.resetSettings();
            Dir dir = LittleFS.openDir("/");
            while (dir.next())
            {   // Only deletes files not folders?
                // Serial.println(dir.fileName());
                LittleFS.remove(dir.fileName());
            }
            digitalWrite(LED_BUILTIN, HIGH);
            while (digitalRead(TRIGGER_PIN) == LOW)
            { // Wait until button is released
                delay(1000);
            }
            ESP.reset();
            delay(5000);
        }
    }
    else
    {
        digitalWrite(LED_BUILTIN, HIGH);
        resetCounter = millis();
    }
}