#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "config.h"

class WiFiManagerHandler {
private:
    WiFiManager wifiManager;
    static bool shouldSaveConfig;
    unsigned long lastReconnectAttempt;
    
    // Callback functions
    static void saveConfigCallback();
    static void configModeCallback(WiFiManager *myWiFiManager);
    static void wifiConnectedCallback();
    
    // mDNS functions
    bool setupMDNS();
    
public:
    WiFiManagerHandler();
    
    // Main functions
    bool initialize();
    bool autoConnect();
    bool startConfigPortal();
    void handleWiFi();
    bool isConnected();
    
    // Status functions
    String getSSID();
    String getIP();
    int getRSSI();
    String getMACAddress();
    String getHostname();
    String getNetworkInfo(bool asJson = false);
    
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
