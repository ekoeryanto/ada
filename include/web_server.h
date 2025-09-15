#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// First include WiFiManager to avoid conflicts with AsyncWebServer HTTP method enums
#include "wifi_manager.h"

#include <ArduinoJson.h>
#include "config.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "modbus_manager.h"

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
    
    // Dummy functions to maintain compatibility (WebSocket removed)
    void broadcastToWebSocket(const String& message) { /* WebSocket removed */ }
    void broadcastSensorData() { /* WebSocket removed */ }
    int getWebSocketClientCount() { return 0; /* WebSocket removed */ }
};

// Global instance declaration
extern WebServerHandler webServer;

#endif // WEB_SERVER_H
