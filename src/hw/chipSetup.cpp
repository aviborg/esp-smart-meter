#include "chipSetup.h"
#include <LittleFS.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

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
    htmlString += "/\">http://";
    htmlString += hostName;
    htmlString += "/</a><br/>You may change the hostname to a custom name, (Only numbers and lowercase letters, no spaces). ";
    htmlString += "Note that the hostname may take a up to 60 minutes to get registered on your local network.";
    WiFiManagerParameter hostNameParam("hostName", hostName, hostName, HOSTNAME_MAXLENGTH);
    WiFiManagerParameter htmlParam(htmlString.c_str());
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    ticker.attach(0.6, tick);

    wifiManager.addParameter(&htmlParam);
    wifiManager.addParameter(&hostNameParam);
    wifiManager.setTimeout(600U);
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
    WiFi.setHostname(hostName);
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
                Serial.println(dir.fileName());
                LittleFS.remove(dir.fileName());
            }
            ESP.eraseConfig();
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

void setupOTA()
{
    // Set OTA hostname
    ArduinoOTA.setHostname(hostName);
    
    // Configure OTA callbacks for better user experience
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_FS
            type = "filesystem";
            // Unmount filesystem if updating filesystem
            LittleFS.end();
        }
        Serial.println("Start updating " + type);
        // Turn on LED to indicate OTA is starting
        digitalWrite(LED_BUILTIN, LOW);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
        // Turn off LED when done
        digitalWrite(LED_BUILTIN, HIGH);
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        // Blink LED during update
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 100) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            lastBlink = millis();
        }
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
        // Blink rapidly to indicate error
        for (int i = 0; i < 10; i++) {
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
        }
    });
    
    ArduinoOTA.begin();
    Serial.println("OTA Ready");
    Serial.print("Hostname: ");
    Serial.println(hostName);
}