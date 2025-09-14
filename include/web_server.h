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
    AsyncWebSocket ws;
    bool serverStarted;
    
    // Helper functions
    void setupRoutes();
    void setupWebSocket();
    String generateWebPage();
    
    // WebSocket functions
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                         AwsEventType type, void *arg, uint8_t *data, size_t len);
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    void notifyWebSocketClients(const String& message);
    
public:
    WebServerHandler();
    bool initialize();
    void begin();  // Start the web server
    void end();    // Stop the web server
    void handle();
    bool isRunning();
    AsyncWebServer* getServer();
    
    // WebSocket functions for external access
    void broadcastToWebSocket(const String& message);
    void broadcastSensorData();
    int getWebSocketClientCount();
};

// Global instance declaration
extern WebServerHandler webServer;

#endif // WEB_SERVER_H
