#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WiFi.h>
#include <Update.h>
#include <ArduinoOTA.h>

#include "config.h"

class OTAHandler {
private:
    AsyncWebServer* server;  // Use AsyncWebServer with ElegantOTA async mode
    bool otaEnabled;
    bool updateInProgress;
    
    // Helper functions
    void setupOTACallbacks();
    
public:
    OTAHandler();
    
    // Main functions
    bool initialize(AsyncWebServer* webServer);  // Use AsyncWebServer
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
