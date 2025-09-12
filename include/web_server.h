#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// First include WiFiManager to avoid conflicts with AsyncWebServer HTTP method enums
#include "wifi_manager.h"

#include <ArduinoJson.h>
#include "config.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// Forward declarations
class OTAHandler;

class WebServerHandler {
private:
    AsyncWebServer server;
    bool serverStarted;
    
    // Helper functions
    void setupRoutes();
    String generateWebPage();
    
public:
    WebServerHandler();
    bool initialize();
    void begin();  // Start the web server
    void end();    // Stop the web server
    void handle();
    bool isRunning();
    AsyncWebServer* getServer();
};

// Global instance declaration
extern WebServerHandler webServer;

#endif // WEB_SERVER_H
