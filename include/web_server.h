#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "wifi_manager.h"
#include "ota_handler.h"

class WebServerHandler {
private:
    ESP8266WebServer server;
    bool serverStarted;
    
    // Route handlers
    void setupRoutes();
    void handleRoot();
    void handleStatus();
    void handleConfig();
    void handleRestart();
    void handleReset();
    void handleNotFound();
    
    // Helper functions
    String generateWebPage();
    
public:
    WebServerHandler();
    
    // Main functions
    bool initialize();
    void begin();
    void end();
    void handle();
    
    // Status functions
    bool isRunning();
    ESP8266WebServer* getServer();
};

// Global instance
extern WebServerHandler webServer;

#endif // WEB_SERVER_H
