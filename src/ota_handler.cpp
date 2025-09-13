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
    
    // Initialize ElegantOTA in async mode with AsyncWebServer (for web interface)
    ElegantOTA.begin(server, OTA_USERNAME, OTA_PASSWORD);
    ElegantOTA.setAutoReboot(true);
    
    // Initialize ArduinoOTA (for command line uploads)
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(3232); // Standard ESP32 OTA port
    
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("[ArduinoOTA] Start updating " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[ArduinoOTA] End");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[ArduinoOTA] Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[ArduinoOTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    
    ArduinoOTA.begin();
    
    otaEnabled = true;
    Serial.println("[OTA] OTA handler initialized successfully");
    Serial.printf("[OTA] Web OTA URL: http://%s/update\n", WiFi.localIP().toString().c_str());
    Serial.printf("[OTA] Web Username: %s\n", OTA_USERNAME);
    Serial.printf("[OTA] Command line OTA: IP %s, Port 3232, Password: %s\n", WiFi.localIP().toString().c_str(), OTA_PASSWORD);
    
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
    
    // Handle ArduinoOTA for command line uploads
    ArduinoOTA.handle();
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
