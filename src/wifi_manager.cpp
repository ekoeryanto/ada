#include "wifi_manager.h"
#include "system_manager.h"
#include <ArduinoJson.h>

// Global instance
WiFiManagerHandler wifiMgr;

// Static members for callbacks
bool WiFiManagerHandler::shouldSaveConfig = false;

WiFiManagerHandler::WiFiManagerHandler() {
    lastReconnectAttempt = 0;
}

bool WiFiManagerHandler::initialize() {
    Serial.println("[WiFiMgr] Initializing WiFi Manager...");
    
    // Set callbacks
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setAPCallback(configModeCallback);
    
    // Configure WiFiManager
    wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
    wifiManager.setConnectTimeout(WIFI_TIMEOUT / 1000);
    wifiManager.setDebugOutput(DEBUG_ENABLED);
    
    // Set custom AP name
    String apName = String(HOSTNAME) + "_" + String(ESP.getChipId(), HEX);
    
    // Set hostname
    WiFi.hostname(HOSTNAME);
    
    Serial.println("[WiFiMgr] WiFi Manager initialized");
    return true;
}

bool WiFiManagerHandler::autoConnect() {
    Serial.println("[WiFiMgr] Attempting auto-connect...");
    systemMgr.setStatus(SYSTEM_WIFI_CONNECTING);
    
    String apName = String(HOSTNAME) + "_" + String(ESP.getChipId(), HEX);
    
    if (wifiManager.autoConnect(apName.c_str(), AP_PASSWORD)) {
        Serial.println("[WiFiMgr] WiFi connected successfully!");
        Serial.printf("[WiFiMgr] IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFiMgr] SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("[WiFiMgr] RSSI: %d dBm\n", WiFi.RSSI());
        Serial.printf("[WiFiMgr] MAC Address: %s\n", WiFi.macAddress().c_str());
        
        // Set hostname again after connection
        WiFi.hostname(HOSTNAME);
        
        // Setup mDNS if enabled
        if (MDNS_ENABLED) {
            setupMDNS();
        }
        
        // Call connected callback
        wifiConnectedCallback();
        
        systemMgr.setStatus(SYSTEM_WIFI_CONNECTED);
        return true;
    } else {
        Serial.println("[WiFiMgr] Failed to connect to WiFi");
        systemMgr.setStatus(SYSTEM_WIFI_FAILED);
        return false;
    }
}

bool WiFiManagerHandler::startConfigPortal() {
    Serial.println("[WiFiMgr] Starting configuration portal...");
    
    String apName = String(HOSTNAME) + "_CONFIG_" + String(ESP.getChipId(), HEX);
    
    if (wifiManager.startConfigPortal(apName.c_str(), AP_PASSWORD)) {
        Serial.println("[WiFiMgr] Configuration portal completed successfully");
        return true;
    } else {
        Serial.println("[WiFiMgr] Configuration portal failed or timed out");
        return false;
    }
}

void WiFiManagerHandler::handleWiFi() {
    // Check if WiFi is still connected
    if (WiFi.status() != WL_CONNECTED) {
        systemMgr.setStatus(SYSTEM_WIFI_CONNECTING);
        
        // Attempt reconnection with delay
        if (millis() - lastReconnectAttempt > WIFI_RETRY_DELAY) {
            Serial.println("[WiFiMgr] WiFi connection lost, attempting reconnection...");
            WiFi.reconnect();
            lastReconnectAttempt = millis();
        }
    } else if (systemMgr.getStatus() == SYSTEM_WIFI_CONNECTING) {
        // WiFi just connected
        systemMgr.setStatus(SYSTEM_WIFI_CONNECTED);
        Serial.printf("[WiFiMgr] WiFi reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
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

String WiFiManagerHandler::getMACAddress() {
    return WiFi.macAddress();
}

String WiFiManagerHandler::getHostname() {
    return WiFi.hostname();
}

String WiFiManagerHandler::getNetworkInfo(bool asJson) {
    if (asJson) {
        DynamicJsonDocument doc(512);
        doc["connected"] = isConnected();
        doc["ssid"] = getSSID();
        doc["ip"] = getIP();
        doc["rssi"] = getRSSI();
        doc["mac"] = getMACAddress();
        doc["hostname"] = getHostname();
        doc["gateway"] = WiFi.gatewayIP().toString();
        doc["subnet"] = WiFi.subnetMask().toString();
        doc["dns"] = WiFi.dnsIP().toString();
        doc["bssid"] = WiFi.BSSIDstr();
        doc["channel"] = WiFi.channel();
        
        String result;
        serializeJson(doc, result);
        return result;
    } else {
        String info = "SSID: " + getSSID() + "\n";
        info += "IP: " + getIP() + "\n";
        info += "RSSI: " + String(getRSSI()) + " dBm\n";
        info += "MAC: " + getMACAddress() + "\n";
        info += "Hostname: " + getHostname() + "\n";
        info += "Gateway: " + WiFi.gatewayIP().toString() + "\n";
        info += "Subnet: " + WiFi.subnetMask().toString() + "\n";
        info += "DNS: " + WiFi.dnsIP().toString() + "\n";
        info += "Channel: " + String(WiFi.channel());
        return info;
    }
}

void WiFiManagerHandler::setConfigPortalTimeout(int timeout) {
    wifiManager.setConfigPortalTimeout(timeout);
}

void WiFiManagerHandler::addParameter(WiFiManagerParameter* parameter) {
    wifiManager.addParameter(parameter);
}

void WiFiManagerHandler::resetWiFiSettings() {
    Serial.println("[WiFiMgr] Resetting WiFi settings...");
    wifiManager.resetSettings();
}

void WiFiManagerHandler::resetAllSettings() {
    Serial.println("[WiFiMgr] Resetting all settings...");
    wifiManager.resetSettings();
    WiFi.disconnect(true);
    delay(1000);
}

bool WiFiManagerHandler::setupMDNS() {
    if (MDNS.begin(HOSTNAME)) {
        Serial.printf("[WiFiMgr] mDNS responder started. Device accessible at http://%s.local\n", HOSTNAME);
        
        // Add service to mDNS
        MDNS.addService(MDNS_SERVICE, MDNS_PROTOCOL, WEB_SERVER_PORT);
        
        return true;
    } else {
        Serial.println("[WiFiMgr] Failed to start mDNS responder");
        return false;
    }
}

// Static callback functions
void WiFiManagerHandler::saveConfigCallback() {
    Serial.println("[WiFiMgr] Configuration should be saved");
    shouldSaveConfig = true;
}

void WiFiManagerHandler::configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("[WiFiMgr] Entered config mode");
    Serial.printf("[WiFiMgr] Config AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("[WiFiMgr] Config AP SSID: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
    
    systemMgr.setStatus(SYSTEM_WIFI_FAILED);
}

void WiFiManagerHandler::wifiConnectedCallback() {
    Serial.println("[WiFiMgr] WiFi connection callback triggered");
    
    // Update full network information
    Serial.println("[WiFiMgr] Extended Network Information:");
    
    // Since this is static, we need to create a temporary info string
    String info = "SSID: " + WiFi.SSID() + "\n";
    info += "IP: " + WiFi.localIP().toString() + "\n";
    info += "RSSI: " + String(WiFi.RSSI()) + " dBm\n";
    info += "MAC: " + WiFi.macAddress() + "\n";
    info += "Hostname: " + WiFi.hostname() + "\n";
    info += "Gateway: " + WiFi.gatewayIP().toString() + "\n";
    info += "Subnet: " + WiFi.subnetMask().toString();
    
    Serial.println(info);
}
