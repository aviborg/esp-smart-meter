#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

class UpdateManager {
public:
    UpdateManager();
    void begin();
    void checkForUpdates();
    bool isUpdateAvailable();
    bool performUpdate();
    String getCurrentVersion();
    String getLatestVersion();
    String getUpdateStatus();
    
private:
    bool fetchLatestRelease();
    int compareVersions(String v1, String v2);
    
    String currentVersion;
    String latestVersion;
    String downloadUrl;
    bool updateAvailable;
    bool updateCheckInProgress;
    unsigned long lastUpdateCheck;
    String updateStatus;
    
    static const unsigned long UPDATE_CHECK_INTERVAL = 3600000; // 1 hour in milliseconds
    static const char* GITHUB_PAGES_VERSION_URL; // Firmware URL is read from version.json
};

#endif
