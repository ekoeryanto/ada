#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WiFi.h>
#include "config.h"

class OTAHandler {
private:
    ESP8266WebServer* server;
    ESP8266HTTPUpdateServer* updateServer;
    bool otaEnabled;
    bool updateInProgress;
    
    // Helper functions
    void setupOTACallbacks();
    
public:
    OTAHandler();
    
    // Main functions
    bool initialize(ESP8266WebServer* webServer);
    void handle();
    void begin();
    void end();
    
    // Status functions
    bool isEnabled();
    bool isUpdateInProgress();
    
    // Configuration functions
    void enable();
    void disable();
    
    // Info functions
    String getUpdateURL();
    String getStatus();
};

// Global instance
extern OTAHandler otaHandler;

#endif // OTA_HANDLER_H
