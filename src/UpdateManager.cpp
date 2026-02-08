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

int UpdateManager::compareVersions(String latestVersion, String currentVersion) {
    // Simple version comparison (major.minor.patch)
    // Returns: 1 if latestVersion > currentVersion, -1 if latestVersion < currentVersion, 0 if equal
    
    // Special case: if current version is "dev", always consider update available
    if (currentVersion == "dev") {
        return 1; // latestVersion < currentVersion, so update is available
    }
    
    // Strip pre-release suffixes (e.g., "1.0.0-alpha" -> "1.0.0")
    String latestVersionClean = latestVersion;
    String currentVersionClean = currentVersion;
    
    int dashPos1 = latestVersionClean.indexOf('-');
    if (dashPos1 > 0) {
        latestVersionClean = latestVersionClean.substring(0, dashPos1);
    }
    
    int dashPos2 = currentVersionClean.indexOf('-');
    if (dashPos2 > 0) {
        currentVersionClean = currentVersionClean.substring(0, dashPos2);
    }
    
    int latestVersionMajor = 0, latestVersionMinor = 0, latestVersionPatch = 0;
    int currentVersionMajor = 0, currentVersionMinor = 0, currentVersionPatch = 0;
    
    // Parse versions safely
    int parsed1 = sscanf(latestVersionClean.c_str(), "%d.%d.%d", &latestVersionMajor, &latestVersionMinor, &latestVersionPatch);
    int parsed2 = sscanf(currentVersionClean.c_str(), "%d.%d.%d", &currentVersionMajor, &currentVersionMinor, &currentVersionPatch);
    
    // If either version couldn't be parsed, consider them equal (no update)
    if (parsed1 < 1 || parsed2 < 1) {
        return 0;
    }
    
    if (latestVersionMajor != currentVersionMajor) return (latestVersionMajor > currentVersionMajor) ? 1 : -1;
    if (latestVersionMinor != currentVersionMinor) return (latestVersionMinor > currentVersionMinor) ? 1 : -1;
    if (latestVersionPatch != currentVersionPatch) return (latestVersionPatch > currentVersionPatch) ? 1 : -1;
    
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
    Serial.println("Performing update...");
    updateStatus = "Updating...";
    
    // Give the ESP some breathing room before starting heavy operations
    yield();
    delay(100);
    
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    ESPhttpUpdate.rebootOnUpdate(false); // Don't auto-reboot, we'll handle it
    
    // CRITICAL FIX: GitHub Pages may negotiate HTTP/2 via ALPN during TLS handshake
    // ESP8266HTTPUpdate library only supports HTTP/1.1 request/response format
    // 
    // Using WiFiClientSecure with setInsecure() should prevent ALPN negotiation in BearSSL
    // However, if server still sends HTTP/2, the library will fail
    // 
    // WORKAROUND: The ESP8266 Core 3.0.2's BearSSL doesn't advertise ALPN when setInsecure() is used
    // This should force GitHub Pages CDN to fall back to HTTP/1.1
    static WiFiClientSecure client;
    
    // Stop any existing connection to ensure clean state
    client.stop();
    
    // Configure for this update
    client.setInsecure(); // Disables cert validation; BearSSL won't advertise ALPN/h2
    client.setTimeout(120000); // 2 minutes timeout for slow HTTPS downloads
    client.setBufferSizes(1024, 1024); // Adequate buffers for HTTPS
    
    // Give ESP time to configure the client
    yield();
    delay(100);
    
    // Use HTTPS URL as-is (GitHub Pages requires HTTPS, redirects HTTP to HTTPS)
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
            ESP.restart(); // Manual reboot after successful update
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
