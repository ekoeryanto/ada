/**
 * 0x3 ESP8266 Project
 * 
 * A modern, well-structured ESP8266 project featuring:
 * - WiFi Manager for easy configuration
 * - OTA (Over-The-Air) updates
 * - Web-based control panel
 * - Modular and maintainable code structure
 * - Professional branding and UI
 * 
 * Author: 0x3
 * Version: 1.0.0
 * 
 * Hardware: Wemos D1 Mini (ESP8266)
 * 
 * Features:
 * - Automatic WiFi connection with fallback to AP mode
 * - Web interface for status monitoring and control
 * - OTA updates via web interface
 * - System status indication via built-in LED
 * - RESTful API for system information
 * - Factory reset and WiFi reset capabilities
 * - Responsive web design for mobile and desktop
 */

#include <Arduino.h>
#include "config.h"
#include "system_manager.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "ota_handler.h"

// Global variables for timing
unsigned long lastStatusUpdate = 0;
unsigned long lastWiFiCheck = 0;

void setup() {
    // Initialize system manager first
    systemMgr.initialize();
    systemMgr.setStatus(SYSTEM_INITIALIZING);
    
    Serial.println("[Main] Starting 0x3 ESP Project...");
    
    // Initialize WiFi Manager
    if (!wifiMgr.initialize()) {
        Serial.println("[Main] Failed to initialize WiFi Manager!");
        systemMgr.setStatus(SYSTEM_ERROR);
        return;
    }
    
    // Attempt WiFi connection
    Serial.println("[Main] Attempting WiFi connection...");
    if (wifiMgr.autoConnect()) {
        Serial.println("[Main] WiFi connected successfully!");
        systemMgr.setStatus(SYSTEM_WIFI_CONNECTED);
        
        // Initialize web server
        if (!webServer.initialize()) {
            Serial.println("[Main] Failed to initialize web server!");
            systemMgr.setStatus(SYSTEM_ERROR);
            return;
        }
        
        // Start web server
        webServer.begin();
        
        // Initialize OTA handler
        if (!otaHandler.initialize(webServer.getServer())) {
            Serial.println("[Main] Failed to initialize OTA handler!");
        } else {
            Serial.println("[Main] OTA handler initialized successfully");
        }
        
        systemMgr.setStatus(SYSTEM_RUNNING);
        Serial.println("[Main] System initialization complete!");
        Serial.printf("[Main] Web interface: http://%s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[Main] OTA updates: http://%s/update\n", WiFi.localIP().toString().c_str());
        
    } else {
        Serial.println("[Main] WiFi connection failed!");
        systemMgr.setStatus(SYSTEM_WIFI_FAILED);
        
        // Start configuration portal
        Serial.println("[Main] Starting configuration portal...");
        if (wifiMgr.startConfigPortal()) {
            Serial.println("[Main] Configuration completed, restarting...");
            delay(2000);
            systemMgr.restart();
        } else {
            Serial.println("[Main] Configuration portal failed or timed out");
            systemMgr.setStatus(SYSTEM_ERROR);
        }
    }
    
    Serial.println("[Main] Setup completed");
}

void loop() {
    // System manager loop (handles LED status updates)
    systemMgr.loop();
    
    // Handle WiFi connection monitoring
    if (millis() - lastWiFiCheck > 5000) { // Check every 5 seconds
        wifiMgr.handleWiFi();
        lastWiFiCheck = millis();
    }
    
    // Handle web server requests
    webServer.handle();
    
    // Handle OTA updates
    otaHandler.handle();
    
    // Status updates
    if (millis() - lastStatusUpdate > STATUS_UPDATE_INTERVAL) {
        // Update system status based on WiFi connection
        if (wifiMgr.isConnected() && systemMgr.getStatus() != SYSTEM_RUNNING && 
            systemMgr.getStatus() != SYSTEM_OTA_UPDATE) {
            systemMgr.setStatus(SYSTEM_RUNNING);
        }
        
        lastStatusUpdate = millis();
    }
    
    // Small delay to prevent watchdog timeout
    delay(10);
}
