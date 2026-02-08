#include "UpdateManager.h"
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include "version.h"

const char* UpdateManager::GITHUB_PAGES_VERSION_URL = "https://aviborg.github.io/esp-smart-meter/firmware/version.json";

UpdateManager::UpdateManager() 
    : currentVersion(FIRMWARE_VERSION)
    , latestVersion("")
    , downloadUrl("")
    , updateAvailable(false)
    , updateCheckInProgress(false)
    , lastUpdateCheck(0)
    , updateStatus("Not checked")
{
}

void UpdateManager::begin() {
    Serial.println("UpdateManager initialized");
    Serial.print("Current firmware version: ");
    Serial.println(currentVersion);
}

void UpdateManager::checkForUpdates() {
    unsigned long now = millis();
    
    // Check if enough time has passed since last check
    if (now - lastUpdateCheck < UPDATE_CHECK_INTERVAL && lastUpdateCheck != 0) {
        return;
    }
    
    // Don't start a new check if one is already in progress
    if (updateCheckInProgress) {
        return;
    }
    
    updateCheckInProgress = true;
    lastUpdateCheck = now;
    
    Serial.println("Checking for firmware updates...");
    updateStatus = "Checking...";
    
    if (fetchLatestRelease()) {
        if (compareVersions(latestVersion, currentVersion) > 0) {
            updateAvailable = true;
            updateStatus = "Update available: " + latestVersion;
            Serial.print("New version available: ");
            Serial.println(latestVersion);
        } else {
            updateAvailable = false;
            updateStatus = "Up to date";
            Serial.println("Firmware is up to date");
        }
    } else {
        updateStatus = "Check failed";
        Serial.println("Failed to check for updates");
    }
    
    updateCheckInProgress = false;
}

bool UpdateManager::fetchLatestRelease() {
    WiFiClientSecure client;
    // WARNING: setInsecure() disables certificate validation
    // This is a security risk as it allows potential MITM attacks
    // For production use, consider:
    // 1. Using certificate fingerprint: client.setFingerprint(...)
    // 2. Using full certificate validation: client.setCACert(...)
    // 3. Using certificate pinning for GitHub Pages
    // The current implementation assumes a trusted local network
    client.setInsecure();
    
    // Set timeout for connection (10 seconds)
    client.setTimeout(10000);
    
    // Set buffer sizes for HTTPS connection
    client.setBufferSizes(512, 512);
    
    HTTPClient https;
    
    // Set timeout for HTTP request (15 seconds)
    https.setTimeout(15000);
    
    // Fetch version information from GitHub Pages
    if (!https.begin(client, GITHUB_PAGES_VERSION_URL)) {
        Serial.println("Failed to connect to GitHub Pages");
        return false;
    }
    
    https.addHeader("User-Agent", "ESP-Smart-Meter");
    int httpCode = https.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("GitHub Pages request failed, error: %d\n", httpCode);
        https.end();
        return false;
    }
    
    String payload = https.getString();
    https.end();
    
    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Extract version information
    const char* version = doc["version"];
    if (!version) {
        Serial.println("No version found in response");
        return false;
    }
    
    latestVersion = String(version);
    
    // Get firmware download URL from version.json
    const char* url = doc["download_url"];
    if (url) {
        downloadUrl = String(url);
    } else {
        Serial.println("No download_url found in version.json");
        return false;
    }
    
    Serial.print("Found firmware version: ");
    Serial.println(latestVersion);
    Serial.print("Download URL: ");
    Serial.println(downloadUrl);
    
    return true;
}

int UpdateManager::compareVersions(String v1, String v2) {
    // Simple version comparison (major.minor.patch)
    // Returns: 1 if v1 > v2, -1 if v1 < v2, 0 if equal
    
    // Special case: if current version is "dev", always consider update available
    if (v1 == "dev") {
        return -1; // v1 < v2, so update is available
    }
    
    // Strip pre-release suffixes (e.g., "1.0.0-alpha" -> "1.0.0")
    String v1Clean = v1;
    String v2Clean = v2;
    
    int dashPos1 = v1Clean.indexOf('-');
    if (dashPos1 > 0) {
        v1Clean = v1Clean.substring(0, dashPos1);
    }
    
    int dashPos2 = v2Clean.indexOf('-');
    if (dashPos2 > 0) {
        v2Clean = v2Clean.substring(0, dashPos2);
    }
    
    int v1Major = 0, v1Minor = 0, v1Patch = 0;
    int v2Major = 0, v2Minor = 0, v2Patch = 0;
    
    // Parse versions safely
    int parsed1 = sscanf(v1Clean.c_str(), "%d.%d.%d", &v1Major, &v1Minor, &v1Patch);
    int parsed2 = sscanf(v2Clean.c_str(), "%d.%d.%d", &v2Major, &v2Minor, &v2Patch);
    
    // If either version couldn't be parsed, consider them equal (no update)
    if (parsed1 < 1 || parsed2 < 1) {
        return 0;
    }
    
    if (v1Major != v2Major) return (v1Major > v2Major) ? 1 : -1;
    if (v1Minor != v2Minor) return (v1Minor > v2Minor) ? 1 : -1;
    if (v1Patch != v2Patch) return (v1Patch > v2Patch) ? 1 : -1;
    
    return 0;
}

bool UpdateManager::isUpdateAvailable() {
    return updateAvailable;
}

bool UpdateManager::performUpdate() {
    if (!updateAvailable || downloadUrl.length() == 0) {
        Serial.println("No update available or download URL not set");
        updateStatus = "No update available";
        return false;
    }
    
    Serial.println("Starting firmware update...");
    Serial.print("Downloading from: ");
    Serial.println(downloadUrl);
    updateStatus = "Updating...";
    
    // Give the ESP some breathing room before starting heavy operations
    yield();
    delay(100);
    
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    ESPhttpUpdate.rebootOnUpdate(true);
    
    // Create a NEW WiFiClientSecure for this update operation
    // Using a fresh client for each update avoids state issues
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation (GitHub Pages uses valid certs but we skip for simplicity)
    client.setTimeout(120000); // 2 minutes timeout for large files
    client.setBufferSizes(1024, 1024); // Larger buffers for better HTTPS performance
    
    // Give ESP time to set up the client
    yield();
    
    // GitHub Pages serves files directly - no redirects!
    // This is much more reliable than GitHub Releases which redirect to Azure CDN with long URLs
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, downloadUrl, currentVersion);
    
    switch(ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("Update failed. Error (%d): %s\n", 
                ESPhttpUpdate.getLastError(), 
                ESPhttpUpdate.getLastErrorString().c_str());
            updateStatus = "Update failed";
            return false;
            
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("No update needed");
            updateStatus = "No update needed";
            return false;
            
        case HTTP_UPDATE_OK:
            Serial.println("Update successful! Rebooting...");
            updateStatus = "Update successful";
            return true;
    }
    
    return false;
}

String UpdateManager::getCurrentVersion() {
    return currentVersion;
}

String UpdateManager::getLatestVersion() {
    return latestVersion;
}

String UpdateManager::getUpdateStatus() {
    return updateStatus;
}
