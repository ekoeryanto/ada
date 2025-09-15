#include "wifi_manager.h"
#include "system_manager.h"
#include "ota_handler.h"

// Global instance
WiFiManagerHandler wifiMgr;

// Static members for callbacks
bool WiFiManagerHandler::shouldSaveConfig = false;

WiFiManagerHandler::WiFiManagerHandler() {
    lastReconnectAttempt = 0;
}

bool WiFiManagerHandler::initialize() {
    // Serial.println("[WiFiMgr] Initializing WiFi Manager...");
    
    // Set callbacks
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setAPCallback(configModeCallback);
    
    // Configure WiFiManager
    wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
    wifiManager.setConnectTimeout(WIFI_TIMEOUT / 1000);
    wifiManager.setDebugOutput(DEBUG_ENABLED);
    
    // Set custom AP name
    String apName = String(HOSTNAME) + "_" + String(systemMgr.getChipId(), HEX);
    
    // Set hostname
    WiFi.setHostname(HOSTNAME);
    
    // Serial.println("[WiFiMgr] WiFi Manager initialized");
    return true;
}

bool WiFiManagerHandler::autoConnect() {
    Serial.println("[WiFiMgr] Attempting auto-connect...");
    systemMgr.setStatus(SYSTEM_WIFI_CONNECTING);
    
    String apName = String(HOSTNAME) + "_" + String(systemMgr.getChipId(), HEX);
    
    if (wifiManager.autoConnect(apName.c_str(), AP_PASSWORD)) {
        Serial.println("[WiFiMgr] WiFi connected successfully!");
        Serial.printf("[WiFiMgr] IP Address: %s\n", WiFi.localIP().toString().c_str());
        // Serial.printf("[WiFiMgr] SSID: %s\n", WiFi.SSID().c_str());
        // Serial.printf("[WiFiMgr] RSSI: %d dBm\n", WiFi.RSSI());
        
        systemMgr.setStatus(SYSTEM_WIFI_CONNECTED);
        return true;
    } else {
        Serial.println("[WiFiMgr] Failed to connect to WiFi");
        systemMgr.setStatus(SYSTEM_WIFI_FAILED);
        return false;
    }
}

bool WiFiManagerHandler::quickConnect(unsigned long timeoutMs) {
    Serial.println("[WiFiMgr] Attempting quick WiFi connection...");
    systemMgr.setStatus(SYSTEM_WIFI_CONNECTING);
    
    // Try to connect to saved credentials first
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeoutMs) {
        delay(100);
        systemMgr.loop();  // Keep system running
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFiMgr] Quick WiFi connection successful!");
        Serial.printf("[WiFiMgr] IP Address: %s\n", WiFi.localIP().toString().c_str());
        systemMgr.setStatus(SYSTEM_WIFI_CONNECTED);
        return true;
    } else {
        Serial.println("[WiFiMgr] Quick WiFi connection failed - will retry in background");
        systemMgr.setStatus(SYSTEM_WIFI_CONNECTING);
        return false;
    }
}

bool WiFiManagerHandler::startConfigPortal() {
    // Serial.println("[WiFiMgr] Starting configuration portal...");
    
    String apName = String(HOSTNAME) + "_CONFIG_" + String(systemMgr.getChipId(), HEX);
    
    if (wifiManager.startConfigPortal(apName.c_str(), AP_PASSWORD)) {
        // Serial.println("[WiFiMgr] Configuration portal completed successfully");
        return true;
    } else {
        // Serial.println("[WiFiMgr] Configuration portal failed or timed out");
        return false;
    }
}

void WiFiManagerHandler::handleWiFi() {
    // Skip WiFi reconnection during OTA update to avoid interruption
    if (otaHandler.isUpdateInProgress()) {
        return;
    }
    
    // Check if WiFi is still connected
    if (WiFi.status() != WL_CONNECTED) {
        systemMgr.setStatus(SYSTEM_WIFI_CONNECTING);
        
        // Attempt reconnection with delay
        if (millis() - lastReconnectAttempt > WIFI_RETRY_DELAY) {
            // Serial.println("[WiFiMgr] WiFi connection lost, attempting reconnection...");
            WiFi.reconnect();
            lastReconnectAttempt = millis();
        }
    } else if (systemMgr.getStatus() == SYSTEM_WIFI_CONNECTING) {
        // WiFi just connected
        systemMgr.setStatus(SYSTEM_WIFI_CONNECTED);
        // Serial.printf("[WiFiMgr] WiFi reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
    }
}

bool WiFiManagerHandler::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManagerHandler::getSSID() {
    return WiFi.SSID();
}

String WiFiManagerHandler::getIP() {
    return WiFi.localIP().toString();
}

int WiFiManagerHandler::getRSSI() {
    return WiFi.RSSI();
}

void WiFiManagerHandler::setConfigPortalTimeout(int timeout) {
    wifiManager.setConfigPortalTimeout(timeout);
}

void WiFiManagerHandler::addParameter(WiFiManagerParameter* parameter) {
    wifiManager.addParameter(parameter);
}

void WiFiManagerHandler::resetWiFiSettings() {
    // Serial.println("[WiFiMgr] Resetting WiFi settings...");
    wifiManager.resetSettings();
}

void WiFiManagerHandler::resetAllSettings() {
    // Serial.println("[WiFiMgr] Resetting all settings...");
    wifiManager.resetSettings();
    WiFi.disconnect(true);
    delay(1000);
}

// Static callback functions
void WiFiManagerHandler::saveConfigCallback() {
    // Serial.println("[WiFiMgr] Configuration should be saved");
    shouldSaveConfig = true;
}

void WiFiManagerHandler::configModeCallback(WiFiManager *myWiFiManager) {
    // Serial.println("[WiFiMgr] Entered config mode");
    // Serial.printf("[WiFiMgr] Config AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    // Serial.printf("[WiFiMgr] Config AP SSID: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
    
    systemMgr.setStatus(SYSTEM_WIFI_FAILED);
}
