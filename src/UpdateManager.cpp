#include "UpdateManager.h"
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include "version.h"

const char* UpdateManager::GITHUB_API_URL = "https://api.github.com/repos/aviborg/esp-smart-meter/releases/latest";

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
    // 3. Using certificate pinning for GitHub API
    // The current implementation assumes a trusted local network
    client.setInsecure();
    
    // Set timeout for connection (10 seconds)
    client.setTimeout(10000);
    
    // Set buffer sizes for HTTPS connection
    client.setBufferSizes(512, 512);
    
    HTTPClient https;
    
    // Set timeout for HTTP request (15 seconds)
    https.setTimeout(15000);
    
    if (!https.begin(client, GITHUB_API_URL)) {
        Serial.println("Failed to connect to GitHub API");
        return false;
    }
    
    https.addHeader("User-Agent", "ESP-Smart-Meter");
    int httpCode = https.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("GitHub API request failed, error: %d\n", httpCode);
        https.end();
        return false;
    }
    
    String payload = https.getString();
    https.end();
    
    // Parse JSON response
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Extract version tag (e.g., "v1.0.0" or "1.0.0")
    const char* tag = doc["tag_name"];
    if (!tag) {
        Serial.println("No tag_name found in response");
        return false;
    }
    
    latestVersion = String(tag);
    // Remove 'v' prefix if present
    if (latestVersion.startsWith("v") || latestVersion.startsWith("V")) {
        latestVersion = latestVersion.substring(1);
    }
    
    // Find firmware binary in assets
    JsonArray assets = doc["assets"];
    for (JsonObject asset : assets) {
        const char* name = asset["name"];
        if (name && (strstr(name, ".bin") != nullptr)) {
            downloadUrl = asset["browser_download_url"].as<String>();
            Serial.print("Found firmware: ");
            Serial.println(downloadUrl);
            break;
        }
    }
    
    if (downloadUrl.length() == 0) {
        Serial.println("No firmware binary found in release");
        return false;
    }
    
    return true;
}

int UpdateManager::compareVersions(String v1, String v2) {
    // Simple version comparison (major.minor.patch)
    // Returns: 1 if v1 > v2, -1 if v1 < v2, 0 if equal
    
    int v1Major = 0, v1Minor = 0, v1Patch = 0;
    int v2Major = 0, v2Minor = 0, v2Patch = 0;
    
    sscanf(v1.c_str(), "%d.%d.%d", &v1Major, &v1Minor, &v1Patch);
    sscanf(v2.c_str(), "%d.%d.%d", &v2Major, &v2Minor, &v2Patch);
    
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
    
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    ESPhttpUpdate.rebootOnUpdate(true);
    
    // Configure redirect handling for GitHub releases
    ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    // Let the library handle the connection internally
    // This avoids stack issues with WiFiClientSecure going out of scope
    t_httpUpdate_return ret = ESPhttpUpdate.update(downloadUrl);
    
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
