#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFiManager.h>
#include <WiFi.h>
#include "config.h"

class WiFiManagerHandler {
private:
    WiFiManager wifiManager;
    static bool shouldSaveConfig;
    unsigned long lastReconnectAttempt;
    
    // Callback functions
    static void saveConfigCallback();
    static void configModeCallback(WiFiManager *myWiFiManager);
    
public:
    WiFiManagerHandler();
    
    // Main functions
    bool initialize();
    bool autoConnect();
    bool quickConnect(unsigned long timeoutMs = 10000);  // Non-blocking quick connect
    bool startConfigPortal();
    void handleWiFi();
    bool isConnected();
    
    // Status functions
    String getSSID();
    String getIP();
    int getRSSI();
    
    // Configuration functions
    void setConfigPortalTimeout(int timeout);
    void addParameter(WiFiManagerParameter* parameter);
    
    // Reset functions
    void resetWiFiSettings();
    void resetAllSettings();
};

// Global instance
extern WiFiManagerHandler wifiMgr;

#endif // WIFI_MANAGER_H
