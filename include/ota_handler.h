#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Update.h>
#include <ArduinoOTA.h>

#include "config.h"

class OTAHandler {
private:
    AsyncWebServer* server;  // Use AsyncWebServer for simple OTA upload
    bool otaEnabled;
    bool updateInProgress;
    
    // Helper functions
    void setupOTACallbacks();
    void handleOTAResult(AsyncWebServerRequest *request);
    
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
    
    // OTA Upload handler (public for web server access)
    void handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
};

// Global instance
extern OTAHandler otaHandler;

#endif // OTA_HANDLER_H
