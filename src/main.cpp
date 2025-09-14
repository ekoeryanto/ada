/**
 * 0x3 ESP32 Project
 * 
 * A modern, well-structured ESP32 project featuring:
 * - WiFi Manager for easy configuration
 * - OTA (Over-The-Air) updates
 * - Web-based control panel
 * - Modular and maintainable code structure
 * - Professional branding and UI
 * 
 * Author: 0x3
 * Version: 1.0.0
 * 
 * Hardware: ESP32 (board-agnostic)
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
#include "sd_manager.h"
#include "ntp_manager.h"
#include "analog_voltage_manager.h"
#include "analog_current_manager.h"
#include "analytics_manager.h"
#include "remote_diagnostics.h"
#include "webhook_handler.h"

// Global variables for timing
unsigned long lastStatusUpdate = 0;
unsigned long lastWiFiCheck = 0;

void setup() {
    // Initialize system manager first
    systemMgr.initialize();
    systemMgr.setStatus(SYSTEM_INITIALIZING);
    
    Serial.println("[Main] Starting 0x3 ESP Project...");
    
    // Initialize SD Manager early
    // Serial.println("[Main] Initializing SD Manager...");
    if (sdMgr.initialize()) {
        // Serial.println("[Main] SD Manager initialized successfully");
        
        // Run SD card self-test
        // Serial.println("[Main] Running SD card self-test...");
        if (sdMgr.runSelfTest()) {
            // Serial.println("[Main] SD card self-test passed!");
        } else {
            // Serial.println("[Main] SD card self-test failed - continuing without SD");
        }
    } else {
        // Serial.println("[Main] SD Manager initialization failed - continuing without SD");
    }
    
    // Initialize Analog Voltage Manager
    // Serial.println("[Main] Initializing Analog Voltage Manager...");
    if (analogVoltageMgr.begin()) {
        // Serial.println("[Main] Analog Voltage Manager initialized successfully");
        
        // Configure sensors for different applications
        analogVoltageMgr.configureSensor(0, "Pressure Tank 1", "bar", 0.0, 10.0, 2.0, 8.0);
        analogVoltageMgr.configureSensor(1, "Flow Sensor", "L/min", 0.0, 100.0, 10.0, 90.0);
        analogVoltageMgr.configureSensor(2, "Temperature", "°C", 0.0, 100.0, 5.0, 85.0);
        
        // Configure enhanced sensor identity
        analogVoltageMgr.configureSensorIdentity(0, "PRESS_TK1_001", "Honeywell", "ST3000", "HW123456789", 
                                               "2025-09-13", "Main tank pressure transmitter", "Tank_A", "critical,pressure,tank");
        analogVoltageMgr.configureSensorIdentity(1, "FLOW_LINE1_002", "Endress+Hauser", "Promag 50", "EH987654321",
                                               "2025-09-13", "Main line flow meter", "Line_1", "flow,line,production");
        analogVoltageMgr.configureSensorIdentity(2, "TEMP_AMB_003", "Yokogawa", "YTA320", "YG555777888",
                                               "2025-09-13", "Ambient temperature sensor", "Environment", "temperature,ambient,monitoring");
        
        // Configure alarm settings
        analogVoltageMgr.setAlarmConfig(0, 3, "CRITICAL: Tank pressure abnormal!", true);  // Critical alarm
        analogVoltageMgr.setAlarmConfig(1, 2, "WARNING: Flow rate deviation detected", true);  // Warning
        analogVoltageMgr.setAlarmConfig(2, 1, "INFO: Temperature monitoring", true);  // Info level
        
        // Configure advanced filtering for noise reduction
        // Pressure sensor: moderate smoothing, outlier detection enabled
        analogVoltageMgr.setSmoothingFactor(0, 0.2);  // Less responsive, more stable for pressure
        analogVoltageMgr.enableOutlierDetection(0, true, 15.0);  // 15% outlier threshold
        
        // Flow sensor: more responsive smoothing, outlier detection for spikes
        analogVoltageMgr.setSmoothingFactor(1, 0.4);  // More responsive for flow changes
        analogVoltageMgr.enableOutlierDetection(1, true, 25.0);  // 25% outlier threshold for flow spikes
        
        // Temperature sensor: heavy smoothing, outlier detection enabled
        analogVoltageMgr.setSmoothingFactor(2, 0.1);  // Very smooth for temperature
        analogVoltageMgr.enableOutlierDetection(2, true, 10.0);  // 10% outlier threshold for stable temp
        
        // Serial.println("[Main] Advanced filtering configured:");
        // Serial.println("[Main] - Pressure: EMA α=0.2, Outlier=15%");
        // Serial.println("[Main] - Flow: EMA α=0.4, Outlier=25%");
        // Serial.println("[Main] - Temperature: EMA α=0.1, Outlier=10%");
        
        // Set reading interval to 2 seconds
        analogVoltageMgr.setReadInterval(2000);
        
        // Set logging interval to 5 minutes (300 seconds)
        analogVoltageMgr.setLogInterval(300000);
        
        // Enable data logging if SD card is available
        analogVoltageMgr.enableLogging(sdMgr.isMounted());
        
        // Serial.println("[Main] Analog voltage sensors configured:");
        // Serial.println("[Main] - Sensor 0: Pressure Tank 1 (AI1) - 0-10 bar [PRESS_TK1_001]");
        // Serial.println("[Main] - Sensor 1: Flow Sensor (AI2) - 0-100 L/min [FLOW_LINE1_002]");
        // Serial.println("[Main] - Sensor 2: Temperature (AI3) - 0-100 °C [TEMP_AMB_003]");
        // Serial.println("[Main] - All sensors have enhanced identity and alarm configuration");
    } else {
        Serial.println("[Main] Analog Voltage Manager initialization failed!");
    }
    
    // Initialize Analog Current Manager for 4-20mA sensors
    Serial.println("[Main] Initializing Analog Current Manager...");
    if (analogCurrentMgr.begin()) {
        Serial.println("[Main] Analog Current Manager initialized successfully");
        
        // Configure current loop sensors for different applications
        analogCurrentMgr.configureSensor(0, "Level Tank 2", "mm", 0.0, 5000.0, 119.0, 2.0, 500.0, 4500.0);
        analogCurrentMgr.configureSensor(1, "Flow Line 2", "L/min", 0.0, 200.0, 120.0, 2.0, 20.0, 180.0);
        analogCurrentMgr.configureSensor(2, "Pressure Sys", "bar", 0.0, 16.0, 120.0, 2.0, 2.0, 14.0);
        
        // Configure enhanced sensor identity for current loops
        analogCurrentMgr.configureSensorIdentity(0, "LEVEL_TK2_004", "Rosemount", "3051L", "RM789123456", 
                                               "2025-09-14", "Tank 2 level transmitter (4-20mA)", "Tank_B", "critical,level,tank,4-20ma");
        analogCurrentMgr.configureSensorIdentity(1, "FLOW_LINE2_005", "Yokogawa", "ADMAG AE", "YG123789456",
                                               "2025-09-14", "Secondary line flow meter (4-20mA)", "Line_2", "flow,line,backup,4-20ma");
        analogCurrentMgr.configureSensorIdentity(2, "PRESS_SYS_006", "Endress+Hauser", "Cerabar PMC21", "EH456123789",
                                               "2025-09-14", "System pressure transmitter (4-20mA)", "System", "pressure,system,main,4-20ma");
        
        // Configure current loop parameters
        analogCurrentMgr.configureCurrentLoop(0, 4.0, 20.0, 119.0, 2.0);  // Tank level with custom sense resistor
        analogCurrentMgr.configureCurrentLoop(1, 4.0, 20.0, 120.0, 2.0);  // Flow meter standard config
        analogCurrentMgr.configureCurrentLoop(2, 4.0, 20.0, 120.0, 2.0);  // Pressure transmitter standard config
        
        // Configure loop diagnostics for health monitoring
        analogCurrentMgr.configureLoopDiagnostics(0, true, 250.0, 15.0);  // Level sensor: expected 250Ω ±15%
        analogCurrentMgr.configureLoopDiagnostics(1, true, 300.0, 20.0);  // Flow meter: expected 300Ω ±20% (longer cable)
        analogCurrentMgr.configureLoopDiagnostics(2, true, 200.0, 10.0);  // Pressure: expected 200Ω ±10% (short cable)
        
        // Configure alarm settings for current loops
        analogCurrentMgr.setAlarmConfig(0, 4, "EMERGENCY: Tank level critical!", true);  // Emergency level alarm
        analogCurrentMgr.setAlarmConfig(1, 2, "WARNING: Secondary flow deviation", true);  // Warning flow alarm
        analogCurrentMgr.setAlarmConfig(2, 3, "CRITICAL: System pressure abnormal!", true);  // Critical pressure alarm
        
        // Configure advanced filtering for current sensors
        analogCurrentMgr.setSmoothingFactor(0, 0.15);  // Heavy smoothing for level (slow changes)
        analogCurrentMgr.enableOutlierDetection(0, true, 12.0);  // 12% outlier threshold for level
        
        analogCurrentMgr.setSmoothingFactor(1, 0.35);  // Moderate smoothing for flow
        analogCurrentMgr.enableOutlierDetection(1, true, 20.0);  // 20% outlier threshold for flow variations
        
        analogCurrentMgr.setSmoothingFactor(2, 0.25);  // Moderate smoothing for pressure
        analogCurrentMgr.enableOutlierDetection(2, true, 15.0);  // 15% outlier threshold for pressure
        
        // Set timing configuration
        analogCurrentMgr.setReadInterval(1500);    // 1.5 seconds for current loops
        analogCurrentMgr.setLogInterval(300000);   // 5 minutes logging interval
        
        // Enable data logging and streaming
        analogCurrentMgr.enableLogging(sdMgr.isMounted());
        analogCurrentMgr.enableStreaming(true);
        
        Serial.println("[Main] Analog current sensors configured:");
        Serial.println("[Main] - Sensor 0: Level Tank 2 (AI1) - 0-5000 mm [LEVEL_TK2_004]");
        Serial.println("[Main] - Sensor 1: Flow Line 2 (AI2) - 0-200 L/min [FLOW_LINE2_005]");
        Serial.println("[Main] - Sensor 2: Pressure Sys (AI3) - 0-16 bar [PRESS_SYS_006]");
        Serial.println("[Main] - All current loops have diagnostics and health monitoring enabled");
    } else {
        Serial.println("[Main] Analog Current Manager initialization failed!");
    }
    
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
        
        // Initialize NTP Manager after WiFi connection
        // Serial.println("[Main] Initializing NTP Manager...");
        if (ntpMgr.initialize()) {
            // Serial.println("[Main] NTP Manager initialized successfully");
        } else {
            // Serial.println("[Main] NTP Manager initialization deferred - will retry when WiFi is stable");
        }
        
        // Initialize web server
        if (!webServer.initialize()) {
            Serial.println("[Main] Failed to initialize web server!");
            systemMgr.setStatus(SYSTEM_ERROR);
            return;
        }
        
        // Start web server
        webServer.begin();
        
        // Initialize OTA handler
        if (!otaHandler.initialize(webServer.getServer())) {  // Uses AsyncWebServer
            Serial.println("[Main] Failed to initialize OTA handler!");
        } else {
            // Serial.println("[Main] OTA handler initialized successfully");
        }
        
        systemMgr.setStatus(SYSTEM_RUNNING);
        Serial.println("[Main] System initialization complete!");
        Serial.printf("[Main] Web interface: http://%s\n", WiFi.localIP().toString().c_str());
        // Serial.printf("[Main] OTA updates: %s\n", otaHandler.getUpdateURL().c_str());
        
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
    
    // Initialize Analytics Manager
    Serial.println("[Main] Initializing Analytics Manager...");
    if (analyticsMgr.begin()) {
        Serial.println("[Main] Analytics Manager initialized successfully");
        
        // Configure analytics
        AnalyticsConfig analyticsConfig;
        analyticsConfig.enabled = true;
        analyticsConfig.analysisInterval = 30000;      // 30 seconds
        analyticsConfig.trendPeriod = 3600000;         // 1 hour
        analyticsConfig.minSamplesForAnalysis = 10;
        analyticsConfig.enablePrediction = true;
        analyticsConfig.enableOutlierDetection = true;
        analyticsConfig.outlierThreshold = 2.5;        // 2.5 sigma
        analyticsConfig.enableDataCompression = true;
        
        analyticsMgr.setAnalyticsConfig(analyticsConfig);
        Serial.println("[Main] Analytics configured: 30s analysis, 1h trends, prediction enabled");
    } else {
        Serial.println("[Main] Analytics Manager initialization failed!");
    }
    
    // Initialize Remote Diagnostics
    Serial.println("[Main] Initializing Remote Diagnostics...");
    if (remoteDiag.begin()) {
        Serial.println("[Main] Remote Diagnostics initialized successfully");
        
        // Configure diagnostics
        remoteDiag.setDiagnosticInterval(60000);  // 1 minute
        remoteDiag.enableRemoteCommands(true);
        
        Serial.println("[Main] Remote diagnostics configured: 1min interval, remote commands enabled");
    } else {
        Serial.println("[Main] Remote Diagnostics initialization failed!");
    }
    
    // Initialize Webhook Handler
    Serial.println("[Main] Initializing Webhook Handler...");
    if (webhookHandler.begin()) {
        Serial.println("[Main] Webhook Handler initialized successfully");
        
        // Add default webhook configuration (can be configured via API)
        // String webhookId = webhookHandler.addWebhook("http://your-server.com/webhook", "your-secret");
        // Serial.printf("[Main] Default webhook added: %s\n", webhookId.c_str());
        
        Serial.println("[Main] Webhook handler ready for configuration via API");
    } else {
        Serial.println("[Main] Webhook Handler initialization failed!");
    }
    
    Serial.println("[Main] Setup completed");
}

void loop() {
    // System manager loop (handles LED status updates)
    systemMgr.loop();
    
    // Handle SD Manager (hot-plug detection, etc.)
    sdMgr.handle();
    
    // Handle NTP Manager (time synchronization)
    ntpMgr.handle();
    
    // Handle Analog Voltage Manager (sensor readings)
    analogVoltageMgr.handle();
    
    // Handle Analog Current Manager (4-20mA sensor readings)
    analogCurrentMgr.handle();
    
    // Handle Analytics Manager (data analysis and trends)
    analyticsMgr.handle();
    
    // Handle Remote Diagnostics (system health monitoring)
    remoteDiag.handle();
    
    // Handle Webhook Handler (outbound data reporting)
    webhookHandler.handle();
    
    // Feed sensor data to analytics
    static unsigned long lastAnalyticsUpdate = 0;
    if (millis() - lastAnalyticsUpdate > 5000) {  // Every 5 seconds
        // Feed voltage sensor data to analytics
        for (int i = 0; i < 3; i++) {
            if (analogVoltageMgr.isSensorEnabled(i)) {
                AnalogReading reading = analogVoltageMgr.getReading(i);
                if (reading.valid) {
                    analyticsMgr.addDataPoint(i, reading.scaledValue, reading.timestamp);
                }
            }
        }
        
        // Feed current sensor data to analytics (offset sensor IDs by 3)
        for (int i = 0; i < 3; i++) {
            if (analogCurrentMgr.isSensorEnabled(i)) {
                CurrentReading reading = analogCurrentMgr.getReading(i);
                if (reading.valid) {
                    analyticsMgr.addDataPoint(i + 3, reading.scaledValue, reading.timestamp);
                }
            }
        }
        lastAnalyticsUpdate = millis();
    }
    
    // Send periodic sensor data via webhooks
    static unsigned long lastWebhookUpdate = 0;
    if (millis() - lastWebhookUpdate > 30000) {  // Every 30 seconds
        for (int i = 0; i < 3; i++) {
            if (analogVoltageMgr.isSensorEnabled(i)) {
                AnalogReading reading = analogVoltageMgr.getReading(i);
                if (reading.valid) {
                    String unit = analogVoltageMgr.getUnit(i);
                    webhookHandler.sendSensorData(
                        "sensor_" + String(i), 
                        reading.scaledValue, 
                        unit, 
                        reading.timestamp
                    );
                }
            }
        }
        lastWebhookUpdate = millis();
    }
    
    // Handle WiFi connection monitoring
    if (millis() - lastWiFiCheck > 5000) { // Check every 5 seconds
        wifiMgr.handleWiFi();
        lastWiFiCheck = millis();
    }
    
    // Handle OTA updates
    if (systemMgr.getStatus() == SYSTEM_RUNNING || systemMgr.getStatus() == SYSTEM_WIFI_CONNECTED) {
        otaHandler.handle();
    }
    
    // Handle web server (AsyncWebServer handles this automatically)
    webServer.handle();
    
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
