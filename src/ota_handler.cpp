#include "ota_handler.h"
#include "system_manager.h"

// Global instance
OTAHandler otaHandler;

OTAHandler::OTAHandler() {
    server = nullptr;
    otaEnabled = false;
    updateInProgress = false;
}

bool OTAHandler::initialize(ESP8266WebServer* webServer) {
    if (!webServer) {
        Serial.println("[OTA] Error: WebServer pointer is null");
        return false;
    }
    
    server = webServer;
    updateServer = new ESP8266HTTPUpdateServer();
    
    Serial.println("[OTA] Initializing OTA handler...");
    
    // Initialize HTTPUpdateServer with authentication
    updateServer->setup(server, "/update", OTA_USERNAME, OTA_PASSWORD);
    
    // Set up status tracking
    updateInProgress = false;
    systemMgr.setStatus(SYSTEM_READY);
    Serial.println("[OTA] Update server initialized");
    
    // Add a status update callback that the web server can use
    server->on("/update/status", HTTP_GET, [this]() {
        String status = getStatus();
        String json = "{\"status\":\"" + status + "\",\"enabled\":" + String(otaEnabled ? "true" : "false") + "}";
        server->send(200, "application/json", json);
    });
    
    otaEnabled = true;
    Serial.println("[OTA] OTA handler initialized successfully");
    Serial.printf("[OTA] OTA URL: http://%s/update\n", WiFi.localIP().toString().c_str());
    Serial.printf("[OTA] Username: %s\n", OTA_USERNAME);
    
    return true;
}

void OTAHandler::setupOTACallbacks() {
    // Callbacks are set up in the initialize method
}

void OTAHandler::handle() {
    // ElegantOTA handles requests asynchronously - no need to call handleClient()
    // This method is kept for compatibility with the existing architecture
}

void OTAHandler::begin() {
    if (server) {
        otaEnabled = true;
        Serial.println("[OTA] OTA service started");
    }
}

void OTAHandler::end() {
    otaEnabled = false;
    Serial.println("[OTA] OTA service stopped");
}

bool OTAHandler::isEnabled() {
    return otaEnabled;
}

bool OTAHandler::isUpdateInProgress() {
    return updateInProgress;
}

void OTAHandler::enable() {
    otaEnabled = true;
    Serial.println("[OTA] OTA enabled");
}

void OTAHandler::disable() {
    otaEnabled = false;
    Serial.println("[OTA] OTA disabled");
}

String OTAHandler::getUpdateURL() {
    if (WiFi.status() == WL_CONNECTED) {
        return "http://" + WiFi.localIP().toString() + "/update";
    }
    return "Not connected to WiFi";
}

String OTAHandler::getStatus() {
    if (!otaEnabled) {
        return "Disabled";
    } else if (updateInProgress) {
        return "Update in progress";
    } else {
        return "Ready";
    }
}
