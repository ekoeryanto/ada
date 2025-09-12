#include "ota_handler.h"
#include "system_manager.h"

// Global instance
OTAHandler otaHandler;

OTAHandler::OTAHandler() {
    server = nullptr;
    otaEnabled = false;
    updateInProgress = false;
}

bool OTAHandler::initialize(AsyncWebServer* webServer) {
    if (!webServer) {
        Serial.println("[OTA] Error: WebServer pointer is null");
        return false;
    }
    
    server = webServer;
    
    Serial.println("[OTA] Initializing OTA handler...");
    
    // Initialize ElegantOTA in async mode with AsyncWebServer
    ElegantOTA.begin(server, OTA_USERNAME, OTA_PASSWORD);
    ElegantOTA.setAutoReboot(true);
    
    otaEnabled = true;
    Serial.println("[OTA] OTA handler initialized successfully");
    Serial.printf("[OTA] OTA URL: http://%s/update\n", WiFi.localIP().toString().c_str());
    Serial.printf("[OTA] Username: %s\n", OTA_USERNAME);
    
    return true;
}

void OTAHandler::setupOTACallbacks() {
    // ElegantOTA has built-in callbacks for progress, start, end events
    ElegantOTA.onStart([]() {
        Serial.println("[OTA] Update Start");
    });
    
    ElegantOTA.onProgress([](size_t current, size_t total) {
        Serial.printf("[OTA] Progress: %u%%\r", (current / (total / 100)));
    });
    
    ElegantOTA.onEnd([](bool success) {
        if (success) {
            Serial.println("\n[OTA] Update finished successfully!");
        } else {
            Serial.println("\n[OTA] Update failed!");
        }
    });
}

void OTAHandler::handle() {
    // ElegantOTA in async mode handles requests automatically
    // Just need to call loop for any background tasks
    ElegantOTA.loop();
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
