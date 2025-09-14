#include "web_server.h"
#include "ota_handler.h"
#include "system_manager.h"
#include "sd_manager.h"
#include "ntp_manager.h"
#include "analog_voltage_manager.h"
#include "analog_current_manager.h"
#include "digital_io_manager.h"
#include "analytics_manager.h"
#include "remote_diagnostics.h"
#include "webhook_handler.h"
#include <Preferences.h>

// Preferences instance for persistent storage
Preferences prefs;

// Simple webhook metadata storage
struct WebhookMetadata {
    String id;
    String name;
    String url;
    String method;
    bool enabled;
    unsigned long created_at;
};

static WebhookMetadata webhookMeta[5]; // Max 5 webhooks
static int webhookMetaCount = 0;

// Helper functions for settings storage
void loadSettingsFromStorage(DynamicJsonDocument& doc) {
    prefs.begin("settings", true); // read-only
    
    // Device settings
    doc["device_name"] = prefs.getString("device_name", "ada-1");
    doc["location"] = prefs.getString("location", "Industrial Controller");
    doc["timezone"] = prefs.getString("timezone", "Asia/Jakarta");
    doc["language"] = prefs.getString("language", "en");
    doc["auto_sync_time"] = prefs.getBool("auto_sync_time", true);
    
    // Display settings
    doc["theme"] = prefs.getString("theme", "light");
    doc["chart_refresh"] = prefs.getInt("chart_refresh", 5);
    doc["max_data_points"] = prefs.getInt("max_data_points", 100);
    doc["show_animations"] = prefs.getBool("show_animations", true);
    doc["sound_notifications"] = prefs.getBool("sound_notifications", false);
    
    // Sensor settings
    doc["sample_rate"] = prefs.getFloat("sample_rate", 1.0);
    doc["averaging_window"] = prefs.getInt("averaging_window", 10);
    doc["filter_type"] = prefs.getString("filter_type", "moving_average");
    doc["outlier_detection"] = prefs.getBool("outlier_detection", true);
    doc["auto_calibration"] = prefs.getBool("auto_calibration", false);
    
    // Logging settings
    doc["enable_logging"] = prefs.getBool("enable_logging", true);
    doc["log_interval"] = prefs.getInt("log_interval", 60);
    doc["max_log_size"] = prefs.getInt("max_log_size", 100);
    doc["retention_days"] = prefs.getInt("retention_days", 30);
    doc["compress_logs"] = prefs.getBool("compress_logs", true);
    
    prefs.end();
}

void saveSettingsToStorage(const JsonObject& settings) {
    prefs.begin("settings", false); // read-write
    
    // Save each setting if present in the request
    if (settings.containsKey("device_name")) prefs.putString("device_name", settings["device_name"].as<String>());
    if (settings.containsKey("location")) prefs.putString("location", settings["location"].as<String>());
    if (settings.containsKey("timezone")) prefs.putString("timezone", settings["timezone"].as<String>());
    if (settings.containsKey("language")) prefs.putString("language", settings["language"].as<String>());
    if (settings.containsKey("auto_sync_time")) prefs.putBool("auto_sync_time", settings["auto_sync_time"]);
    
    if (settings.containsKey("theme")) prefs.putString("theme", settings["theme"].as<String>());
    if (settings.containsKey("chart_refresh")) prefs.putInt("chart_refresh", settings["chart_refresh"]);
    if (settings.containsKey("max_data_points")) prefs.putInt("max_data_points", settings["max_data_points"]);
    if (settings.containsKey("show_animations")) prefs.putBool("show_animations", settings["show_animations"]);
    if (settings.containsKey("sound_notifications")) prefs.putBool("sound_notifications", settings["sound_notifications"]);
    
    if (settings.containsKey("sample_rate")) prefs.putFloat("sample_rate", settings["sample_rate"]);
    if (settings.containsKey("averaging_window")) prefs.putInt("averaging_window", settings["averaging_window"]);
    if (settings.containsKey("filter_type")) prefs.putString("filter_type", settings["filter_type"].as<String>());
    if (settings.containsKey("outlier_detection")) prefs.putBool("outlier_detection", settings["outlier_detection"]);
    if (settings.containsKey("auto_calibration")) prefs.putBool("auto_calibration", settings["auto_calibration"]);
    
    if (settings.containsKey("enable_logging")) prefs.putBool("enable_logging", settings["enable_logging"]);
    if (settings.containsKey("log_interval")) prefs.putInt("log_interval", settings["log_interval"]);
    if (settings.containsKey("max_log_size")) prefs.putInt("max_log_size", settings["max_log_size"]);
    if (settings.containsKey("retention_days")) prefs.putInt("retention_days", settings["retention_days"]);
    if (settings.containsKey("compress_logs")) prefs.putBool("compress_logs", settings["compress_logs"]);
    
    prefs.end();
}

// Helper functions for webhook metadata storage
void loadWebhookMetaFromStorage() {
    prefs.begin("webhooks", true); // read-only
    
    webhookMetaCount = prefs.getInt("count", 0);
    
    for (int i = 0; i < webhookMetaCount && i < 5; i++) {
        String prefix = "wh" + String(i) + "_";
        webhookMeta[i].id = prefs.getString((prefix + "id").c_str(), "");
        webhookMeta[i].name = prefs.getString((prefix + "name").c_str(), "");
        webhookMeta[i].url = prefs.getString((prefix + "url").c_str(), "");
        webhookMeta[i].method = prefs.getString((prefix + "method").c_str(), "POST");
        webhookMeta[i].enabled = prefs.getBool((prefix + "enabled").c_str(), true);
        webhookMeta[i].created_at = prefs.getULong((prefix + "created").c_str(), 0);
    }
    
    prefs.end();
    Serial.printf("[WEBHOOK] Loaded %d webhooks from storage\n", webhookMetaCount);
}

void saveWebhookMetaToStorage() {
    prefs.begin("webhooks", false); // read-write
    
    prefs.putInt("count", webhookMetaCount);
    
    for (int i = 0; i < webhookMetaCount && i < 5; i++) {
        String prefix = "wh" + String(i) + "_";
        prefs.putString((prefix + "id").c_str(), webhookMeta[i].id);
        prefs.putString((prefix + "name").c_str(), webhookMeta[i].name);
        prefs.putString((prefix + "url").c_str(), webhookMeta[i].url);
        prefs.putString((prefix + "method").c_str(), webhookMeta[i].method);
        prefs.putBool((prefix + "enabled").c_str(), webhookMeta[i].enabled);
        prefs.putULong((prefix + "created").c_str(), webhookMeta[i].created_at);
    }
    
    prefs.end();
    Serial.printf("[WEBHOOK] Saved %d webhooks to storage\n", webhookMetaCount);
}

// Global instance
WebServerHandler webServer;

WebServerHandler::WebServerHandler() 
    : server(WEB_SERVER_PORT), ws("/ws")
{
    serverStarted = false;
}

bool WebServerHandler::initialize() {
    // Serial.println("[WebServer] Initializing web server...");
    
    // Load webhook metadata from storage
    loadWebhookMetaFromStorage();
    
    setupRoutes();
    setupWebSocket();
    
    // Serial.printf("[WebServer] Web server initialized on port %d with WebSocket support\n", WEB_SERVER_PORT);
    return true;
}

void WebServerHandler::setupRoutes() {
    // Add CORS headers to all responses
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    
    // Handle OPTIONS requests for CORS
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404, "text/plain", "Not found");
        }
    });
    
    // AsyncWebServer routes
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String html = generateWebPage();
        request->send(200, "text/html", html);
    });
    
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(2048);  // Increased size for water level data
        doc["project"] = PROJECT_NAME;
        doc["version"] = PROJECT_VERSION;
        doc["author"] = PROJECT_AUTHOR;
        doc["status"] = systemMgr.getStatusString();
        doc["uptime"] = systemMgr.getUptimeString();
        doc["freeHeap"] = ESP.getFreeHeap();
        doc["chipId"] = systemMgr.getChipId();
        
        if (wifiMgr.isConnected()) {
            doc["wifi"]["connected"] = true;
            doc["wifi"]["ssid"] = wifiMgr.getSSID();
            doc["wifi"]["ip"] = wifiMgr.getIP();
            doc["wifi"]["rssi"] = wifiMgr.getRSSI();
        } else {
            doc["wifi"]["connected"] = false;
        }
        
        doc["ota"]["enabled"] = otaHandler.isEnabled();
        doc["ota"]["status"] = otaHandler.getStatus();
        doc["ota"]["url"] = otaHandler.getUpdateURL();
        
        // SD card status
        doc["sd"]["mounted"] = sdMgr.isMounted();
        if (sdMgr.isMounted()) {
            // Safely get SD card info with timeout protection
            try {
                doc["sd"]["total_mb"] = sdMgr.getTotalBytes() / (1024 * 1024);
                doc["sd"]["used_mb"] = sdMgr.getUsedBytes() / (1024 * 1024);
                doc["sd"]["card_type"] = sdMgr.getCardType();
            } catch (...) {
                doc["sd"]["error"] = "Unable to read card info";
            }
        }
        
        // NTP/Time status
        doc["ntp"]["initialized"] = ntpMgr.isInitialized();
        doc["ntp"]["synced"] = ntpMgr.isSynced();
        doc["ntp"]["last_sync"] = ntpMgr.getLastSyncTime();
        doc["ntp"]["current_time"] = ntpMgr.getCurrentTimeString();
        doc["ntp"]["rtc_available"] = ntpMgr.isRTCAvailable();
        
        // Analog voltage sensors status
        doc["analog_voltage"]["initialized"] = analogVoltageMgr.isInitialized();
        doc["analog_voltage"]["total_readings"] = analogVoltageMgr.getTotalReadings();
        doc["analog_voltage"]["alarm_status"] = analogVoltageMgr.getAlarmStatus();
        doc["analog_voltage"]["has_errors"] = analogVoltageMgr.hasErrors();
        
        for (int i = 0; i < 3; i++) {
            String sensorKey = "sensor_" + String(i);
            doc["analog_voltage"]["sensors"][sensorKey]["location"] = analogVoltageMgr.getLocation(i);
            doc["analog_voltage"]["sensors"][sensorKey]["enabled"] = analogVoltageMgr.isSensorEnabled(i);
            doc["analog_voltage"]["sensors"][sensorKey]["value"] = analogVoltageMgr.getScaledValue(i);
            doc["analog_voltage"]["sensors"][sensorKey]["unit"] = analogVoltageMgr.getUnit(i);
            doc["analog_voltage"]["sensors"][sensorKey]["voltage"] = analogVoltageMgr.getVoltage(i);
            doc["analog_voltage"]["sensors"][sensorKey]["raw_voltage"] = analogVoltageMgr.getRawVoltage(i);
            doc["analog_voltage"]["sensors"][sensorKey]["status"] = analogVoltageMgr.getStatusString(i);
            doc["analog_voltage"]["sensors"][sensorKey]["health_score"] = analogVoltageMgr.getSensorHealth(i);
            doc["analog_voltage"]["sensors"][sensorKey]["is_dead"] = analogVoltageMgr.isSensorDead(i);
            doc["analog_voltage"]["sensors"][sensorKey]["is_stuck"] = analogVoltageMgr.isSensorStuck(i);
            doc["analog_voltage"]["sensors"][sensorKey]["is_calibrated"] = analogVoltageMgr.isCalibrated(i);
            doc["analog_voltage"]["sensors"][sensorKey]["low_alarm"] = analogVoltageMgr.isLowValue(i);
            doc["analog_voltage"]["sensors"][sensorKey]["high_alarm"] = analogVoltageMgr.isHighValue(i);
            doc["analog_voltage"]["sensors"][sensorKey]["errors"] = analogVoltageMgr.getErrorCount(i);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Health status API endpoint
    server.on("/api/health", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        
        SystemHealthStatus health = systemMgr.getSystemHealth();
        
        // Convert enum values to strings for JSON
        auto healthToString = [](ModuleHealth h) -> String {
            switch(h) {
                case HEALTH_OK: return "OK";
                case HEALTH_WARNING: return "WARNING";
                case HEALTH_ERROR: return "ERROR";
                default: return "UNKNOWN";
            }
        };
        
        doc["wifi"] = healthToString(health.wifi);
        doc["modbus"] = healthToString(health.modbus);
        doc["webserver"] = healthToString(health.webserver);
        doc["sd"] = healthToString(health.sd);
        doc["ntp"] = healthToString(health.ntp);
        doc["ota"] = healthToString(health.ota);
        doc["analytics"] = healthToString(health.analytics);
        doc["diagnostics"] = healthToString(health.diagnostics);
        doc["overall_healthy"] = health.systemHealthy;
        doc["last_check"] = health.lastHealthCheck;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Analog Voltage API endpoint
    server.on("/api/analog-voltage", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        
        doc["initialized"] = analogVoltageMgr.isInitialized();
        doc["total_readings"] = analogVoltageMgr.getTotalReadings();
        doc["alarm_status"] = analogVoltageMgr.getAlarmStatus();
        doc["has_errors"] = analogVoltageMgr.hasErrors();
        
        for (int i = 0; i < 3; i++) {
            AnalogReading reading = analogVoltageMgr.getReading(i);
            String sensorKey = "ai" + String(i + 1);  // ai1, ai2, ai3
            
            doc["sensors"][sensorKey]["location"] = analogVoltageMgr.getLocation(i);
            doc["sensors"][sensorKey]["enabled"] = analogVoltageMgr.isSensorEnabled(i);
            doc["sensors"][sensorKey]["value"] = reading.scaledValue;
            doc["sensors"][sensorKey]["unit"] = analogVoltageMgr.getUnit(i);
            doc["sensors"][sensorKey]["voltage"] = reading.calibratedVoltage;
            doc["sensors"][sensorKey]["raw_adc"] = reading.rawADC;
            doc["sensors"][sensorKey]["raw_voltage"] = reading.voltage;
            doc["sensors"][sensorKey]["status"] = analogVoltageMgr.getStatusString(i);
            doc["sensors"][sensorKey]["valid"] = reading.valid;
            doc["sensors"][sensorKey]["timestamp"] = reading.timestamp;
            doc["sensors"][sensorKey]["low_alarm"] = analogVoltageMgr.isLowValue(i);
            doc["sensors"][sensorKey]["high_alarm"] = analogVoltageMgr.isHighValue(i);
            doc["sensors"][sensorKey]["errors"] = analogVoltageMgr.getErrorCount(i);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Sensor health monitoring endpoint
    server.on("/api/analog-voltage/health", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1536);
        
        doc["timestamp"] = millis();
        doc["system_health"] = "OK";
        
        for (int i = 0; i < 3; i++) {
            String sensorKey = "ai" + String(i + 1);  // ai1, ai2, ai3
            
            doc["sensors"][sensorKey]["location"] = analogVoltageMgr.getLocation(i);
            doc["sensors"][sensorKey]["health_score"] = analogVoltageMgr.getSensorHealth(i);
            doc["sensors"][sensorKey]["is_dead"] = analogVoltageMgr.isSensorDead(i);
            doc["sensors"][sensorKey]["is_stuck"] = analogVoltageMgr.isSensorStuck(i);
            doc["sensors"][sensorKey]["is_calibrated"] = analogVoltageMgr.isCalibrated(i);
            doc["sensors"][sensorKey]["error_count"] = analogVoltageMgr.getErrorCount(i);
            doc["sensors"][sensorKey]["total_readings"] = analogVoltageMgr.getTotalReadings();
            
            // Calculate error rate
            float errorRate = 0.0;
            if (analogVoltageMgr.getTotalReadings() > 0) {
                errorRate = (float)analogVoltageMgr.getErrorCount(i) / analogVoltageMgr.getTotalReadings() * 100.0;
            }
            doc["sensors"][sensorKey]["error_rate_percent"] = errorRate;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Calibration endpoint
    server.on("/api/analog-voltage/calibrate", HTTP_POST, [this](AsyncWebServerRequest *request) {
        String body = "";
        if (request->hasParam("body", true)) {
            body = request->getParam("body", true)->value();
        }
        
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        if (!doc.containsKey("sensor") || !doc.containsKey("offset") || !doc.containsKey("gain")) {
            request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
            return;
        }
        
        int sensor = doc["sensor"];
        float offset = doc["offset"];
        float gain = doc["gain"];
        
        if (sensor < 0 || sensor >= 3) {
            request->send(400, "application/json", "{\"error\":\"Invalid sensor index\"}");
            return;
        }
        
        analogVoltageMgr.setCalibration(sensor, offset, gain);
        
        DynamicJsonDocument response(256);
        response["success"] = true;
        response["sensor"] = sensor;
        response["offset"] = offset;
        response["gain"] = gain;
        response["message"] = "Calibration updated successfully";
        
        String responseStr;
        serializeJson(response, responseStr);
        request->send(200, "application/json", responseStr);
    });
    
    // Reset calibration endpoint
    server.on("/api/analog-voltage/reset-calibration", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("sensor", true)) {
            request->send(400, "application/json", "{\"error\":\"Missing sensor parameter\"}");
            return;
        }
        
        int sensor = request->getParam("sensor", true)->value().toInt();
        
        if (sensor < 0 || sensor >= 3) {
            request->send(400, "application/json", "{\"error\":\"Invalid sensor index\"}");
            return;
        }
        
        analogVoltageMgr.resetCalibration(sensor);
        
        DynamicJsonDocument response(256);
        response["success"] = true;
        response["sensor"] = sensor;
        response["message"] = "Calibration reset successfully";
        
        String responseStr;
        serializeJson(response, responseStr);
        request->send(200, "application/json", responseStr);
    });
    
    // Sensor information endpoint
    server.on("/api/analog-voltage/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(2048);
        
        doc["timestamp"] = millis();
        doc["system"] = "Analog Voltage Manager";
        doc["version"] = "2.0.0";
        
        for (int i = 0; i < 3; i++) {
            String sensorKey = "ai" + String(i + 1);  // ai1, ai2, ai3
            
            doc["sensors"][sensorKey]["sensor_id"] = analogVoltageMgr.getSensorId(i);
            doc["sensors"][sensorKey]["location"] = analogVoltageMgr.getLocation(i);
            doc["sensors"][sensorKey]["manufacturer"] = analogVoltageMgr.getManufacturer(i);
            doc["sensors"][sensorKey]["model"] = analogVoltageMgr.getModel(i);
            doc["sensors"][sensorKey]["serial_number"] = analogVoltageMgr.getSerialNumber(i);
            doc["sensors"][sensorKey]["installation_date"] = analogVoltageMgr.getInstallationDate(i);
            doc["sensors"][sensorKey]["description"] = analogVoltageMgr.getDescription(i);
            doc["sensors"][sensorKey]["group"] = analogVoltageMgr.getGroup(i);
            doc["sensors"][sensorKey]["tags"] = analogVoltageMgr.getTags(i);
            doc["sensors"][sensorKey]["unit"] = analogVoltageMgr.getUnit(i);
            doc["sensors"][sensorKey]["enabled"] = analogVoltageMgr.isSensorEnabled(i);
            doc["sensors"][sensorKey]["calibrated"] = analogVoltageMgr.isCalibrated(i);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Analog Current (4-20mA) API endpoints
    server.on("/api/analog-current", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern AnalogCurrentManager analogCurrentMgr;
        DynamicJsonDocument doc(1024);
        
        doc["initialized"] = analogCurrentMgr.isInitialized();
        doc["total_readings"] = analogCurrentMgr.getTotalReadings();
        doc["alarm_status"] = analogCurrentMgr.getAlarmStatus();
        doc["has_errors"] = analogCurrentMgr.hasErrors();
        
        for (int i = 0; i < 2; i++) {  // Hardware only has 2 current sensors (ADS1115 AIN0, AIN1)
            CurrentReading reading = analogCurrentMgr.getReading(i);
            String sensorKey = "aci" + String(i + 1);  // aci1, aci2
            
            doc["sensors"][sensorKey]["location"] = analogCurrentMgr.getLocation(i);
            doc["sensors"][sensorKey]["enabled"] = analogCurrentMgr.isSensorEnabled(i);
            doc["sensors"][sensorKey]["value"] = reading.scaledValue;
            doc["sensors"][sensorKey]["unit"] = analogCurrentMgr.getUnit(i);
            doc["sensors"][sensorKey]["current"] = reading.current;
            doc["sensors"][sensorKey]["voltage"] = reading.voltage;
            doc["sensors"][sensorKey]["raw_adc"] = reading.rawADC;
            doc["sensors"][sensorKey]["status"] = analogCurrentMgr.getStatusString(i);
            doc["sensors"][sensorKey]["valid"] = reading.valid;
            doc["sensors"][sensorKey]["timestamp"] = reading.timestamp;
            doc["sensors"][sensorKey]["loop_resistance"] = reading.loopResistance;
            doc["sensors"][sensorKey]["signal_quality"] = reading.signalQuality;
            doc["sensors"][sensorKey]["low_alarm"] = analogCurrentMgr.isLowValue(i);
            doc["sensors"][sensorKey]["high_alarm"] = analogCurrentMgr.isHighValue(i);
            doc["sensors"][sensorKey]["errors"] = analogCurrentMgr.getErrorCount(i);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/analog-current/health", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern AnalogCurrentManager analogCurrentMgr;
        DynamicJsonDocument doc(1024);
        
        doc["initialized"] = analogCurrentMgr.isInitialized();
        doc["total_readings"] = analogCurrentMgr.getTotalReadings();
        doc["alarm_status"] = analogCurrentMgr.getAlarmStatus();
        doc["has_errors"] = analogCurrentMgr.hasErrors();
        
        for (int i = 0; i < 2; i++) {  // Hardware only has 2 current sensors
            CurrentReading reading = analogCurrentMgr.getReading(i);
            String sensorKey = "sensor_" + String(i);
            
            doc["sensors"][sensorKey]["location"] = analogCurrentMgr.getLocation(i);
            doc["sensors"][sensorKey]["enabled"] = analogCurrentMgr.isSensorEnabled(i);
            doc["sensors"][sensorKey]["current"] = reading.current;
            doc["sensors"][sensorKey]["expected_range"] = "4.0-20.0 mA";
            doc["sensors"][sensorKey]["status"] = analogCurrentMgr.getStatusString(i);
            doc["sensors"][sensorKey]["valid"] = reading.valid;
            doc["sensors"][sensorKey]["loop_resistance"] = reading.loopResistance;
            doc["sensors"][sensorKey]["signal_quality"] = reading.signalQuality;
            doc["sensors"][sensorKey]["timestamp"] = reading.timestamp;
            doc["sensors"][sensorKey]["low_alarm"] = analogCurrentMgr.isLowValue(i);
            doc["sensors"][sensorKey]["high_alarm"] = analogCurrentMgr.isHighValue(i);
            doc["sensors"][sensorKey]["errors"] = analogCurrentMgr.getErrorCount(i);
            doc["sensors"][sensorKey]["needs_calibration"] = !analogCurrentMgr.isCalibrated(i);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/analog-current/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern AnalogCurrentManager analogCurrentMgr;
        DynamicJsonDocument doc(1024);
        
        doc["system"] = "Analog Current Manager (4-20mA)";
        doc["initialized"] = analogCurrentMgr.isInitialized();
        doc["total_readings"] = analogCurrentMgr.getTotalReadings();
        
        for (int i = 0; i < 3; i++) {
            String sensorKey = "sensor_" + String(i);
            doc["sensors"][sensorKey]["location"] = analogCurrentMgr.getLocation(i);
            doc["sensors"][sensorKey]["unit"] = analogCurrentMgr.getUnit(i);
            doc["sensors"][sensorKey]["manufacturer"] = analogCurrentMgr.getManufacturer(i);
            doc["sensors"][sensorKey]["model"] = analogCurrentMgr.getModel(i);
            doc["sensors"][sensorKey]["serial_number"] = analogCurrentMgr.getSerialNumber(i);
            doc["sensors"][sensorKey]["sensor_id"] = analogCurrentMgr.getSensorId(i);
            doc["sensors"][sensorKey]["enabled"] = analogCurrentMgr.isSensorEnabled(i);
            doc["sensors"][sensorKey]["calibrated"] = analogCurrentMgr.isCalibrated(i);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/analog-current/diagnostics", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern AnalogCurrentManager analogCurrentMgr;
        DynamicJsonDocument doc(1024);
        
        doc["system"] = "4-20mA Loop Diagnostics";
        doc["initialized"] = analogCurrentMgr.isInitialized();
        
        for (int i = 0; i < 3; i++) {
            CurrentReading reading = analogCurrentMgr.getReading(i);
            String sensorKey = "sensor_" + String(i);
            
            doc["sensors"][sensorKey]["location"] = analogCurrentMgr.getLocation(i);
            doc["sensors"][sensorKey]["current_reading"] = reading.current;
            doc["sensors"][sensorKey]["loop_resistance"] = reading.loopResistance;
            doc["sensors"][sensorKey]["signal_quality"] = reading.signalQuality;
            doc["sensors"][sensorKey]["status"] = analogCurrentMgr.getStatusString(i);
            doc["sensors"][sensorKey]["loop_integrity"] = reading.current >= 3.8 && reading.current <= 20.5 ? "OK" : "FAULT";
            doc["sensors"][sensorKey]["wire_resistance"] = reading.loopResistance > 500 ? "HIGH" : "NORMAL";
            doc["sensors"][sensorKey]["signal_noise"] = reading.signalQuality < 80 ? "HIGH" : "LOW";
            doc["sensors"][sensorKey]["calibration_drift"] = !analogCurrentMgr.isCalibrated(i) ? "DETECTED" : "NONE";
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Digital I/O API endpoints (more specific routes first)
    // Digital IO Control (POST)
    server.on("/api/digital-io/output", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern DigitalIOManager digitalIOMgr;
        
        if (!request->hasParam("output") || !request->hasParam("state")) {
            request->send(400, "application/json", "{\"error\":\"Missing parameters: output, state\"}");
            return;
        }
        
        int outputIndex = request->getParam("output")->value().toInt();
        String stateStr = request->getParam("state")->value();
        stateStr.toUpperCase();
        
        if (outputIndex < 0 || outputIndex >= 4) {
            request->send(400, "application/json", "{\"error\":\"Invalid output index (0-3)\"}");
            return;
        }
        
        if (stateStr == "ON" || stateStr == "1") {
            digitalIOMgr.setOutput(outputIndex, true);
        } else if (stateStr == "OFF" || stateStr == "0") {
            digitalIOMgr.setOutput(outputIndex, false);
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid state (ON/OFF or 1/0)\"}");
            return;
        }
        
        // Return success with current status
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["output"] = outputIndex;
        doc["new_state"] = digitalIOMgr.getOutputState(outputIndex);
        doc["operation_count"] = digitalIOMgr.getOutputOperations(outputIndex);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Digital Outputs Only
    server.on("/api/digital-io/outputs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern DigitalIOManager digitalIOMgr;
        DynamicJsonDocument doc(1024);
        
        doc["system"] = "Digital Outputs";
        doc["total_outputs"] = 4;
        
        for (int i = 0; i < 4; i++) {
            DigitalOutputStatus status = digitalIOMgr.getOutputStatus(i);
            String outputKey = "do" + String(i + 1);  // do1, do2, do3, do4
            
            doc["outputs"][outputKey]["name"] = digitalIOMgr.getOutputName(i);
            doc["outputs"][outputKey]["info"] = digitalIOMgr.getOutputInfo(i);
            doc["outputs"][outputKey]["state"] = digitalIOMgr.getOutputState(i);
            doc["outputs"][outputKey]["physical_state"] = status.physicalState;
            doc["outputs"][outputKey]["current_state"] = status.currentState;
            doc["outputs"][outputKey]["duty_cycle"] = status.pwmDutyCycle;
            doc["outputs"][outputKey]["operations_count"] = status.operationCount;
            doc["outputs"][outputKey]["health_score"] = digitalIOMgr.getOutputHealth(i);
            doc["outputs"][outputKey]["last_operation_time"] = status.lastOperationTime;
            doc["outputs"][outputKey]["state_hold_time"] = status.stateHoldTime;
            doc["outputs"][outputKey]["operation_count"] = status.operationCount;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Digital Inputs Only
    server.on("/api/digital-io/inputs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern DigitalIOManager digitalIOMgr;
        DynamicJsonDocument doc(1024);
        
        doc["system"] = "Digital Inputs";
        doc["total_inputs"] = 4;
        
        for (int i = 0; i < 4; i++) {
            DigitalInputReading reading = digitalIOMgr.getInputReading(i);
            String inputKey = "di" + String(i + 1);  // di1, di2, di3, di4
            
            doc["inputs"][inputKey]["name"] = digitalIOMgr.getInputName(i);
            doc["inputs"][inputKey]["info"] = digitalIOMgr.getInputInfo(i);
            doc["inputs"][inputKey]["state"] = reading.currentState == DI_HIGH ? "HIGH" : "LOW";
            doc["inputs"][inputKey]["valid"] = reading.valid;
            doc["inputs"][inputKey]["pulse_count"] = reading.pulseCount;
            doc["inputs"][inputKey]["total_pulses"] = reading.totalPulses;
            doc["inputs"][inputKey]["state_time"] = reading.stateHoldTime;
            doc["inputs"][inputKey]["last_change"] = reading.lastChangeTime;
            doc["inputs"][inputKey]["alarm_active"] = reading.alarmActive;
            doc["inputs"][inputKey]["timestamp"] = reading.timestamp;
            doc["inputs"][inputKey]["health_score"] = digitalIOMgr.getInputHealth(i);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Complete Digital I/O Status
    server.on("/api/digital-io", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern DigitalIOManager digitalIOMgr;
        DynamicJsonDocument doc(1024);
        
        doc["initialized"] = digitalIOMgr.isInitialized();
        doc["total_inputs"] = 4;
        doc["total_outputs"] = 4;
        
        // Digital Inputs
        for (int i = 0; i < 4; i++) {
            DigitalInputReading reading = digitalIOMgr.getInputReading(i);
            String inputKey = "di" + String(i + 1);  // di1, di2, di3, di4
            
            doc["inputs"][inputKey]["name"] = digitalIOMgr.getInputName(i);
            doc["inputs"][inputKey]["state"] = reading.currentState == DI_HIGH ? "HIGH" : "LOW";
            doc["inputs"][inputKey]["valid"] = reading.valid;
            doc["inputs"][inputKey]["pulse_count"] = reading.pulseCount;
            doc["inputs"][inputKey]["total_pulses"] = reading.totalPulses;
            doc["inputs"][inputKey]["state_time"] = reading.stateHoldTime;
            doc["inputs"][inputKey]["last_change"] = reading.lastChangeTime;
            doc["inputs"][inputKey]["alarm_active"] = reading.alarmActive;
            doc["inputs"][inputKey]["timestamp"] = reading.timestamp;
        }
        
        // Digital Outputs
        for (int i = 0; i < 4; i++) {
            DigitalOutputStatus status = digitalIOMgr.getOutputStatus(i);
            String outputKey = "do" + String(i + 1);  // do1, do2, do3, do4
            
            doc["outputs"][outputKey]["name"] = digitalIOMgr.getOutputName(i);
            doc["outputs"][outputKey]["state"] = status.currentState;
            doc["outputs"][outputKey]["physical_state"] = status.physicalState;
            doc["outputs"][outputKey]["duty_cycle"] = status.pwmDutyCycle;
            doc["outputs"][outputKey]["operations"] = status.operationCount;
            doc["outputs"][outputKey]["last_operation"] = status.lastOperationTime;
            doc["outputs"][outputKey]["current_state"] = status.currentState;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/digital-io/inputs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern DigitalIOManager digitalIOMgr;
        DynamicJsonDocument doc(1024);
        
        doc["system"] = "Digital Inputs";
        doc["total_inputs"] = 4;
        
        for (int i = 0; i < 4; i++) {
            DigitalInputReading reading = digitalIOMgr.getInputReading(i);
            String inputKey = "di" + String(i + 1);  // di1, di2, di3, di4
            
            doc["inputs"][inputKey]["name"] = digitalIOMgr.getInputName(i);
            doc["inputs"][inputKey]["info"] = digitalIOMgr.getInputInfo(i);
            doc["inputs"][inputKey]["current_state"] = reading.currentState == DI_HIGH ? "HIGH" : "LOW";
            doc["inputs"][inputKey]["last_state"] = reading.lastState == DI_HIGH ? "HIGH" : "LOW";
            doc["inputs"][inputKey]["state_hold_time"] = reading.stateHoldTime;
            doc["inputs"][inputKey]["pulse_count"] = reading.pulseCount;
            doc["inputs"][inputKey]["total_pulses"] = reading.totalPulses;
            doc["inputs"][inputKey]["transition_rate"] = 0.0f; // Method not available
            doc["inputs"][inputKey]["health_score"] = digitalIOMgr.getInputHealth(i);
            doc["inputs"][inputKey]["alarm_active"] = reading.alarmActive;
            doc["inputs"][inputKey]["debounce_active"] = reading.debounceActive;
            doc["inputs"][inputKey]["valid"] = reading.valid;
            doc["inputs"][inputKey]["timestamp"] = reading.timestamp;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/digital-io/outputs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern DigitalIOManager digitalIOMgr;
        DynamicJsonDocument doc(1024);
        
        doc["system"] = "Digital Outputs";
        doc["total_outputs"] = 4;
        
        for (int i = 0; i < 4; i++) {
            DigitalOutputStatus status = digitalIOMgr.getOutputStatus(i);
            String outputKey = "do" + String(i + 1);  // do1, do2, do3, do4
            
            doc["outputs"][outputKey]["name"] = digitalIOMgr.getOutputName(i);
            doc["outputs"][outputKey]["info"] = digitalIOMgr.getOutputInfo(i);
            doc["outputs"][outputKey]["state"] = digitalIOMgr.getOutputState(i);
            doc["outputs"][outputKey]["physical_state"] = status.physicalState;
            doc["outputs"][outputKey]["current_state"] = status.currentState;
            doc["outputs"][outputKey]["duty_cycle"] = status.pwmDutyCycle;
            doc["outputs"][outputKey]["operations_count"] = status.operationCount;
            doc["outputs"][outputKey]["health_score"] = digitalIOMgr.getOutputHealth(i);
            doc["outputs"][outputKey]["last_operation_time"] = status.lastOperationTime;
            doc["outputs"][outputKey]["state_hold_time"] = status.stateHoldTime;
            doc["outputs"][outputKey]["operation_count"] = status.operationCount;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Digital Output Control (simplified - using query parameters)
    server.on("/api/digital-io/output", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern DigitalIOManager digitalIOMgr;
        
        if (!request->hasParam("output") || !request->hasParam("state")) {
            request->send(400, "application/json", "{\"error\":\"Missing parameters: output, state\"}");
            return;
        }
        
        int outputIndex = request->getParam("output")->value().toInt();
        String stateStr = request->getParam("state")->value();
        stateStr.toUpperCase();
        
        if (outputIndex < 0 || outputIndex >= 4) {
            request->send(400, "application/json", "{\"error\":\"Invalid output index (0-3)\"}");
            return;
        }
        
        if (stateStr == "ON" || stateStr == "1") {
            digitalIOMgr.setOutput(outputIndex, true);
        } else if (stateStr == "OFF" || stateStr == "0") {
            digitalIOMgr.setOutput(outputIndex, false);
        } else if (stateStr == "PULSE") {
            unsigned long duration = 1000; // Default 1 second
            if (request->hasParam("duration")) {
                duration = request->getParam("duration")->value().toInt();
            }
            digitalIOMgr.pulseOutput(outputIndex, duration);
        } else if (stateStr == "BLINK") {
            digitalIOMgr.setOutputState(outputIndex, DO_BLINK);
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid state (ON/OFF/PULSE/BLINK)\"}");
            return;
        }
        
        DynamicJsonDocument response(256);
        response["success"] = true;
        response["output"] = outputIndex;
        response["state"] = stateStr;
        response["name"] = digitalIOMgr.getOutputName(outputIndex);
        
        String responseStr;
        serializeJson(response, responseStr);
        request->send(200, "application/json", responseStr);
    });
    
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(512);
        doc["hostname"] = HOSTNAME;
        doc["ap_password"] = AP_PASSWORD;
        doc["ota_username"] = OTA_USERNAME;
        doc["web_port"] = WEB_SERVER_PORT;
        doc["debug_enabled"] = DEBUG_ENABLED;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Settings API endpoints
    server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        
        // Load settings from persistent storage
        loadSettingsFromStorage(doc);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
        // This will be called after body is parsed
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        Serial.printf("Settings POST: index=%zu, len=%zu, total=%zu\n", index, len, total);
        
        // Check if this is the complete data
        if (index + len != total) {
            Serial.println("Waiting for more settings data...");
            return; // Wait for more data
        }
        
        // Null-terminate the data
        char* jsonStr = (char*)malloc(len + 1);
        memcpy(jsonStr, data, len);
        jsonStr[len] = '\0';
        
        Serial.println("Received settings data:");
        Serial.println(jsonStr);
        
        DynamicJsonDocument requestDoc(1024);
        DeserializationError error = deserializeJson(requestDoc, jsonStr);
        
        free(jsonStr); // Clean up
        
        if (error) {
            Serial.print("JSON Parse Error in settings: ");
            Serial.println(error.c_str());
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        // Save settings to persistent storage
        saveSettingsToStorage(requestDoc.as<JsonObject>());
        
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["message"] = "Settings saved successfully";
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"message\":\"Restarting...\"}");
        delay(1000);
        systemMgr.restart();
    });
    
    server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"message\":\"Resetting WiFi settings...\"}");
        delay(1000);
        wifiMgr.resetWiFiSettings();
        systemMgr.restart();
    });
    
    // SD Card API endpoints
    server.on("/api/sd/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(512);
        doc["mounted"] = sdMgr.isMounted();
        doc["total_bytes"] = sdMgr.getTotalBytes();
        doc["used_bytes"] = sdMgr.getUsedBytes();
        doc["card_type"] = sdMgr.getCardType();
        doc["card_size"] = sdMgr.getCardSize();
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    server.on("/api/sd/test", HTTP_POST, [this](AsyncWebServerRequest *request) {
        bool testResult = sdMgr.runSelfTest();
        DynamicJsonDocument doc(256);
        doc["test_passed"] = testResult;
        doc["message"] = testResult ? "SD card test passed" : "SD card test failed";
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    server.on("/api/sd/files", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String path = "/";
        if (request->hasParam("path")) {
            path = request->getParam("path")->value();
        }
        
        std::vector<String> files = sdMgr.listDirectory(path);
        DynamicJsonDocument doc(2048);
        JsonArray fileArray = doc.createNestedArray("files");
        
        for (const String& file : files) {
            fileArray.add(file);
        }
        doc["path"] = path;
        doc["count"] = files.size();
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    // Simulation control endpoints
    server.on("/api/simulation/enable", HTTP_POST, [this](AsyncWebServerRequest *request) {
        bool enable = false;
        if (request->hasParam("enable", true)) {
            enable = request->getParam("enable", true)->value() == "true";
        }
        
        extern AnalogVoltageManager analogVoltageMgr;
        analogVoltageMgr.enableSimulation(enable);
        
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["message"] = enable ? "Simulation enabled" : "Simulation disabled";
        doc["simulation_enabled"] = enable;
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    server.on("/api/simulation/sensor", HTTP_POST, [this](AsyncWebServerRequest *request) {
        int sensorIndex = -1;
        String mode = "fixed";
        float value = 50.0;
        float amplitude = 10.0;
        float frequency = 0.1;
        
        if (request->hasParam("sensor", true)) {
            sensorIndex = request->getParam("sensor", true)->value().toInt();
        }
        if (request->hasParam("mode", true)) {
            mode = request->getParam("mode", true)->value();
        }
        if (request->hasParam("value", true)) {
            value = request->getParam("value", true)->value().toFloat();
        }
        if (request->hasParam("amplitude", true)) {
            amplitude = request->getParam("amplitude", true)->value().toFloat();
        }
        if (request->hasParam("frequency", true)) {
            frequency = request->getParam("frequency", true)->value().toFloat();
        }
        
        extern AnalogVoltageManager analogVoltageMgr;
        
        if (sensorIndex >= 0 && sensorIndex < 3) {
            analogVoltageMgr.setSimulationMode(sensorIndex, mode);
            analogVoltageMgr.setSimulationValue(sensorIndex, value);
            
            if (mode != "fixed") {
                analogVoltageMgr.setSimulationPattern(sensorIndex, mode, amplitude, frequency);
            }
            
            DynamicJsonDocument doc(512);
            doc["success"] = true;
            doc["message"] = "Simulation configured for sensor " + String(sensorIndex);
            doc["sensor"] = sensorIndex;
            doc["mode"] = mode;
            doc["value"] = value;
            doc["amplitude"] = amplitude;
            doc["frequency"] = frequency;
            
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid sensor index\"}");
        }
    });
    
    server.on("/api/simulation/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern AnalogVoltageManager analogVoltageMgr;
        
        DynamicJsonDocument doc(1024);
        doc["simulation_enabled"] = analogVoltageMgr.isSimulationEnabled();
        
        JsonArray sensors = doc.createNestedArray("sensors");
        for (int i = 0; i < 3; i++) {
            JsonObject sensor = sensors.createNestedObject();
            sensor["id"] = i;
            sensor["simulated"] = analogVoltageMgr.isSensorSimulated(i);
            sensor["location"] = analogVoltageMgr.getLocation(i);
        }
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    // Analytics endpoints
    server.on("/api/analytics/summary", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern AnalyticsManager analyticsMgr;
        String json = analyticsMgr.getAnalyticsSummary();
        request->send(200, "application/json", json);
    });
    
    server.on("/api/analytics/statistics", HTTP_GET, [this](AsyncWebServerRequest *request) {
        int sensor = -1;
        if (request->hasParam("sensor")) {
            sensor = request->getParam("sensor")->value().toInt();
        }
        
        extern AnalyticsManager analyticsMgr;
        
        if (sensor >= 0 && sensor < 3) {
            String json = analyticsMgr.getStatisticsJSON(sensor);
            request->send(200, "application/json", json);
        } else {
            // Return all sensors
            DynamicJsonDocument doc(2048);
            JsonArray sensors = doc.createNestedArray("sensors");
            
            for (int i = 0; i < 3; i++) {
                DynamicJsonDocument sensorDoc(1024);
                deserializeJson(sensorDoc, analyticsMgr.getStatisticsJSON(i));
                sensors.add(sensorDoc.as<JsonObject>());
            }
            
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        }
    });
    
    server.on("/api/analytics/trends", HTTP_GET, [this](AsyncWebServerRequest *request) {
        int sensor = -1;
        if (request->hasParam("sensor")) {
            sensor = request->getParam("sensor")->value().toInt();
        }
        
        extern AnalyticsManager analyticsMgr;
        
        if (sensor >= 0 && sensor < 3) {
            String json = analyticsMgr.getTrendJSON(sensor);
            request->send(200, "application/json", json);
        } else {
            // Return all sensors
            DynamicJsonDocument doc(2048);
            JsonArray sensors = doc.createNestedArray("sensors");
            
            for (int i = 0; i < 3; i++) {
                DynamicJsonDocument trendDoc(512);
                deserializeJson(trendDoc, analyticsMgr.getTrendJSON(i));
                sensors.add(trendDoc.as<JsonObject>());
            }
            
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        }
    });
    
    server.on("/api/analytics/prediction", HTTP_GET, [this](AsyncWebServerRequest *request) {
        int sensor = -1;
        unsigned long futureTime = millis() + 300000;  // Default: 5 minutes ahead
        
        if (request->hasParam("sensor")) {
            sensor = request->getParam("sensor")->value().toInt();
        }
        if (request->hasParam("time")) {
            futureTime = request->getParam("time")->value().toInt();
        }
        
        extern AnalyticsManager analyticsMgr;
        
        if (sensor >= 0 && sensor < 3) {
            float prediction = analyticsMgr.predictNextValue(sensor, futureTime);
            
            DynamicJsonDocument doc(256);
            doc["sensor"] = sensor;
            doc["predicted_value"] = prediction;
            doc["prediction_time"] = futureTime;
            doc["confidence"] = "medium";  // Placeholder
            
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid sensor index\"}");
        }
    });
    
    // Remote diagnostics endpoints
    server.on("/api/diagnostics/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern RemoteDiagnostics remoteDiag;
        String json = remoteDiag.getStatus();
        request->send(200, "application/json", json);
    });
    
    server.on("/api/diagnostics/all", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern RemoteDiagnostics remoteDiag;
        String json = remoteDiag.getAllDiagnosticsJSON();
        request->send(200, "application/json", json);
    });
    
    server.on("/api/diagnostics/alerts", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern RemoteDiagnostics remoteDiag;
        String json = remoteDiag.getAlertsJSON();
        request->send(200, "application/json", json);
    });
    
    server.on("/api/diagnostics/health", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern RemoteDiagnostics remoteDiag;
        
        DynamicJsonDocument doc(512);
        doc["health_score"] = remoteDiag.getOverallHealthScore();
        doc["status"] = remoteDiag.isSystemHealthy() ? "healthy" : "needs_attention";
        doc["last_check"] = remoteDiag.getLastDiagnosticTime();
        doc["alerts_count"] = remoteDiag.getAlertCount();
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    server.on("/api/diagnostics/clear-alerts", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern RemoteDiagnostics remoteDiag;
        remoteDiag.clearAlerts();
        
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["message"] = "All alerts cleared";
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    // Webhook Management Endpoints
    server.on("/api/webhooks", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern WebhookHandler webhookHandler;
        
        // Create response in format expected by Zod schema
        DynamicJsonDocument doc(2048);
        
        // Create webhooks array from stored metadata
        JsonArray webhooks = doc.createNestedArray("webhooks");
        
        for (int i = 0; i < webhookMetaCount; i++) {
            JsonObject webhook = webhooks.createNestedObject();
            webhook["name"] = webhookMeta[i].name;
            webhook["url"] = webhookMeta[i].url;
            webhook["method"] = webhookMeta[i].method;
            webhook["enabled"] = webhookMeta[i].enabled;
            webhook["created_at"] = webhookMeta[i].created_at;
            // Add some default values expected by client
            webhook["timeout"] = 5000;
            webhook["max_retries"] = 3;
            webhook["headers"] = "{}";
            webhook["payload_template"] = "{}";
        }
        
        // Add statistics object
        JsonObject statistics = doc.createNestedObject("statistics");
        statistics["total_sent"] = webhookHandler.getTotalSent();
        statistics["success_rate"] = webhookHandler.getSuccessRate();
        statistics["queue_size"] = webhookHandler.getQueueSize();
        
        String json;
        serializeJson(doc, json);
        Serial.println("GET /api/webhooks response:");
        Serial.println(json);
        request->send(200, "application/json", json);
    });
    
    server.on("/api/webhooks", HTTP_POST, [this](AsyncWebServerRequest *request) {
        // This will be called after body is parsed
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        extern WebhookHandler webhookHandler;
        
        Serial.printf("Webhook POST: index=%zu, len=%zu, total=%zu\n", index, len, total);
        
        // Check if this is the complete data
        if (index + len != total) {
            Serial.println("Waiting for more data...");
            return; // Wait for more data
        }
        
        // Null-terminate the data
        char* jsonStr = (char*)malloc(len + 1);
        memcpy(jsonStr, data, len);
        jsonStr[len] = '\0';
        
        Serial.println("Received webhook data:");
        Serial.println(jsonStr);
        
        DynamicJsonDocument requestDoc(1024);
        DeserializationError error = deserializeJson(requestDoc, jsonStr);
        
        free(jsonStr); // Clean up
        
        if (error) {
            Serial.print("JSON Parse Error: ");
            Serial.println(error.c_str());
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        Serial.println("Received webhook data:");
        serializeJsonPretty(requestDoc, Serial);
        Serial.println();
        
        // Extract webhook data from JSON
        String name = requestDoc["name"] | "";
        String url = requestDoc["url"] | "";
        String method = requestDoc["method"] | "POST";
        int timeout = requestDoc["timeout"] | 5000;
        bool enabled = requestDoc["enabled"] | true;
        
        Serial.printf("Parsed: name=%s, url=%s, method=%s\n", name.c_str(), url.c_str(), method.c_str());
        
        if (name.length() == 0) {
            request->send(400, "application/json", "{\"error\":\"Name is required\"}");
            return;
        }
        
        if (url.length() == 0) {
            request->send(400, "application/json", "{\"error\":\"URL is required\"}");
            return;
        }
        
        // Remove name validation for now - allow default name
        // if (name.length() == 0) {
        //     request->send(400, "application/json", "{\"error\":\"Name is required\"}");
        //     return;
        // }
        
        // For now, use simplified webhook creation
        String webhookId = webhookHandler.addWebhook(url, "", ""); // secret and authToken empty for now
        
        if (webhookId.length() > 0) {
            // Store metadata
            if (webhookMetaCount < 5) {
                webhookMeta[webhookMetaCount].id = webhookId;
                webhookMeta[webhookMetaCount].name = name;
                webhookMeta[webhookMetaCount].url = url;
                webhookMeta[webhookMetaCount].method = method;
                webhookMeta[webhookMetaCount].enabled = enabled;
                webhookMeta[webhookMetaCount].created_at = millis();
                webhookMetaCount++;
                
                // Save to persistent storage
                saveWebhookMetaToStorage();
            }
            
            DynamicJsonDocument doc(512);
            doc["success"] = true;
            doc["webhook_id"] = webhookId;
            doc["name"] = name;
            doc["url"] = url;
            doc["method"] = method;
            doc["timeout"] = timeout;
            doc["enabled"] = enabled;
            doc["message"] = "Webhook created successfully";
            
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        } else {
            request->send(400, "application/json", "{\"error\":\"Failed to create webhook\"}");
        }
    });
    
    server.on("/api/webhooks/test", HTTP_POST, [this](AsyncWebServerRequest *request) {
        // This will be called after body is parsed
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        extern WebhookHandler webhookHandler;
        
        Serial.printf("Webhook TEST: index=%zu, len=%zu, total=%zu\n", index, len, total);
        
        // Check if this is the complete data
        if (index + len != total) {
            Serial.println("Waiting for more test data...");
            return; // Wait for more data
        }
        
        // Null-terminate the data
        char* jsonStr = (char*)malloc(len + 1);
        memcpy(jsonStr, data, len);
        jsonStr[len] = '\0';
        
        Serial.println("Received webhook test data:");
        Serial.println(jsonStr);
        
        DynamicJsonDocument requestDoc(1024);
        DeserializationError error = deserializeJson(requestDoc, jsonStr);
        
        free(jsonStr); // Clean up
        
        if (error) {
            Serial.print("JSON Parse Error in webhook test: ");
            Serial.println(error.c_str());
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        Serial.println("Received webhook test data:");
        serializeJsonPretty(requestDoc, Serial);
        Serial.println();
        
        String name = requestDoc["name"] | "";
        String id = requestDoc["id"] | "";
        String url = requestDoc["url"] | "";
        String method = requestDoc["method"] | "POST";
        
        Serial.printf("Test webhook: name=%s, id=%s, url=%s, method=%s\n", name.c_str(), id.c_str(), url.c_str(), method.c_str());
        
        // Check if we have either name or id (required for webhook identification)
        if (name.length() == 0 && id.length() == 0) {
            request->send(400, "application/json", "{\"error\":\"Either name or id is required for webhook testing\"}");
            return;
        }
        
        // Check if we have URL (required for testing)
        if (url.length() == 0) {
            request->send(400, "application/json", "{\"error\":\"URL is required for testing\"}");
            return;
        }
        
        // For now, just return success since we don't have actual webhook testing implementation
        bool success = true; // webhookHandler.testWebhookUrl(url, method);
        
        DynamicJsonDocument doc(384);
        doc["success"] = success;
        doc["message"] = success ? "Test webhook sent successfully" : "Failed to send test webhook";
        doc["url"] = url;
        doc["method"] = method;
        if (name.length() > 0) doc["name"] = name;
        if (id.length() > 0) doc["id"] = id;
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    server.on("/api/webhooks/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern WebhookHandler webhookHandler;
        request->send(200, "application/json", webhookHandler.getStatus());
    });
    
    server.on("/api/webhooks/statistics", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern WebhookHandler webhookHandler;
        request->send(200, "application/json", webhookHandler.getStatistics());
    });
    
    server.on("/api/webhooks/queue", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern WebhookHandler webhookHandler;
        request->send(200, "application/json", webhookHandler.getQueueStatus());
    });
    
    server.on("/api/webhooks/queue/clear", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern WebhookHandler webhookHandler;
        webhookHandler.clearQueue();
        
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["message"] = "Webhook queue cleared";
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    server.on("/api/webhooks/queue/retry", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern WebhookHandler webhookHandler;
        webhookHandler.retryFailed();
        
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["message"] = "Retrying failed webhooks";
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // ===== MODBUS MANAGEMENT API ENDPOINTS =====
    // Get all Modbus devices status
    server.on("/api/modbus/devices", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern ModbusManager modbusManager;
        
        DynamicJsonDocument doc(4096);
        doc["initialized"] = modbusManager.isInitialized();
        doc["total_devices"] = modbusManager.getDeviceCount();
        doc["connected_devices"] = modbusManager.getConnectedDeviceCount();
        
        // Network statistics
        ModbusNetworkStats stats = modbusManager.getNetworkStats();
        doc["network"]["success_rate"] = stats.networkSuccessRate;
        doc["network"]["avg_response_time"] = stats.averageResponseTime;
        doc["network"]["total_requests"] = stats.totalRequests;
        doc["network"]["failed_requests"] = stats.failedRequests;
        doc["network"]["active_devices"] = stats.activeDevices;
        
        // Device list
        JsonArray devices = doc.createNestedArray("devices");
        std::vector<uint8_t> connectedDevices = modbusManager.getConnectedDevices();
        
        for (uint8_t slaveId : connectedDevices) {
            JsonObject device = devices.createNestedObject();
            device["slave_id"] = slaveId;
            device["name"] = modbusManager.getDeviceName(slaveId);
            device["status"] = "connected";
            device["last_communication"] = modbusManager.getLastCommunicationTime(slaveId);
            device["error_count"] = modbusManager.getErrorCount(slaveId);
            device["health_score"] = modbusManager.getDeviceHealth(slaveId);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Add new Modbus device
    server.on("/api/modbus/devices", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern ModbusManager modbusManager;
        
        // Parse JSON body
        String body;
        if (request->hasParam("plain", true)) {
            body = request->getParam("plain", true)->value();
        }
        
        DynamicJsonDocument requestDoc(1024);
        deserializeJson(requestDoc, body);
        
        // Create device config from JSON
        ModbusDeviceConfig config;
        config.name = requestDoc["name"].as<String>();
        config.description = requestDoc["description"].as<String>();
        config.slaveId = requestDoc["slave_id"];
        config.baudRate = requestDoc["baud_rate"] | 9600;
        config.dataBits = requestDoc["data_bits"] | 8;
        config.parity = requestDoc["parity"] | 0;
        config.stopBits = requestDoc["stop_bits"] | 1;
        config.enabled = requestDoc["enabled"] | true;
        config.responseTimeout = requestDoc["response_timeout"] | 1000;
        config.frameDelay = requestDoc["frame_delay"] | 100;
        config.retryDelay = requestDoc["retry_delay"] | 500;
        config.maxRetries = requestDoc["max_retries"] | 3;
        config.healthMonitoring = requestDoc["health_monitoring"] | true;
        config.healthInterval = requestDoc["health_interval"] | 30000;
        
        bool success = modbusManager.addDevice(config);
        
        DynamicJsonDocument responseDoc(256);
        responseDoc["success"] = success;
        responseDoc["message"] = success ? "Device added successfully" : "Failed to add device";
        
        String response;
        serializeJson(responseDoc, response);
        request->send(success ? 200 : 400, "application/json", response);
    });
    
    // Get specific device configuration
    server.on("/api/modbus/devices/*", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern ModbusManager modbusManager;
        
        String path = request->url();
        int slaveId = path.substring(path.lastIndexOf('/') + 1).toInt();
        
        if (!modbusManager.deviceExists(slaveId)) {
            request->send(404, "application/json", "{\"error\":\"Device not found\"}");
            return;
        }
        
        DynamicJsonDocument doc(1024);
        doc["slave_id"] = slaveId;
        doc["name"] = modbusManager.getDeviceName(slaveId);
        doc["connected"] = modbusManager.isDeviceConnected(slaveId);
        doc["health_score"] = modbusManager.getDeviceHealth(slaveId);
        doc["error_count"] = modbusManager.getErrorCount(slaveId);
        doc["last_communication"] = modbusManager.getLastCommunicationTime(slaveId);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Remove Modbus device
    server.on("/api/modbus/devices/*", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
        extern ModbusManager modbusManager;
        
        String path = request->url();
        int slaveId = path.substring(path.lastIndexOf('/') + 1).toInt();
        
        bool success = modbusManager.removeDevice(slaveId);
        
        DynamicJsonDocument doc(256);
        doc["success"] = success;
        doc["message"] = success ? "Device removed successfully" : "Failed to remove device";
        
        String response;
        serializeJson(doc, response);
        request->send(success ? 200 : 400, "application/json", response);
    });
    
    // Read register from device
    server.on("/api/modbus/read", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern ModbusManager modbusManager;
        
        String body;
        if (request->hasParam("plain", true)) {
            body = request->getParam("plain", true)->value();
        }
        
        DynamicJsonDocument requestDoc(512);
        deserializeJson(requestDoc, body);
        
        uint8_t slaveId = requestDoc["slave_id"];
        String registerName = requestDoc["register_name"];
        
        ModbusReading reading = modbusManager.readRegister(slaveId, registerName);
        
        DynamicJsonDocument responseDoc(512);
        responseDoc["success"] = reading.valid;
        responseDoc["slave_id"] = slaveId;
        responseDoc["register_name"] = registerName;
        responseDoc["value"] = reading.scaledValue;
        responseDoc["raw_data"] = JsonArray();
        JsonArray rawArray = responseDoc["raw_data"];
        for (size_t i = 0; i < reading.rawData.size(); i++) {
            rawArray.add(reading.rawData[i]);
        }
        responseDoc["unit"] = reading.unit;
        responseDoc["timestamp"] = reading.timestamp;
        responseDoc["quality"] = reading.quality;
        
        String response;
        serializeJson(responseDoc, response);
        request->send(200, "application/json", response);
    });
    
    // Write register to device
    server.on("/api/modbus/write", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern ModbusManager modbusManager;
        
        String body;
        if (request->hasParam("plain", true)) {
            body = request->getParam("plain", true)->value();
        }
        
        DynamicJsonDocument requestDoc(512);
        deserializeJson(requestDoc, body);
        
        uint8_t slaveId = requestDoc["slave_id"];
        uint16_t address = requestDoc["address"];
        uint16_t value = requestDoc["value"];
        
        bool success = modbusManager.writeRegister(slaveId, address, value);
        
        DynamicJsonDocument responseDoc(256);
        responseDoc["success"] = success;
        responseDoc["message"] = success ? "Register written successfully" : "Failed to write register";
        
        String response;
        serializeJson(responseDoc, response);
        request->send(success ? 200 : 400, "application/json", response);
    });
    
    // Auto-discover Modbus devices
    server.on("/api/modbus/discover", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern ModbusManager modbusManager;
        
        String startIdStr = "1";
        String endIdStr = "247";
        
        if (request->hasParam("start_id", true)) {
            startIdStr = request->getParam("start_id", true)->value();
        }
        if (request->hasParam("end_id", true)) {
            endIdStr = request->getParam("end_id", true)->value();
        }
        
        uint8_t startId = startIdStr.toInt();
        uint8_t endId = endIdStr.toInt();
        
        // Start discovery (this should be async in real implementation)
        bool success = modbusManager.startDeviceDiscovery(startId, endId);
        
        DynamicJsonDocument doc(256);
        doc["success"] = success;
        doc["message"] = success ? "Device discovery started" : "Failed to start discovery";
        doc["scan_range"]["start"] = startId;
        doc["scan_range"]["end"] = endId;
        
        String response;
        serializeJson(doc, response);
        request->send(success ? 200 : 400, "application/json", response);
    });
    
    // Get Modbus network statistics
    server.on("/api/modbus/stats", HTTP_GET, [this](AsyncWebServerRequest *request) {
        extern ModbusManager modbusManager;
        
        ModbusNetworkStats stats = modbusManager.getNetworkStats();
        
        DynamicJsonDocument doc(512);
        doc["network_success_rate"] = stats.networkSuccessRate;
        doc["average_response_time"] = stats.averageResponseTime;
        doc["total_requests"] = stats.totalRequests;
        doc["failed_requests"] = stats.failedRequests;
        doc["active_devices"] = stats.activeDevices;
        doc["last_update"] = millis();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.onNotFound([this](AsyncWebServerRequest *request) {
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
    });
}

void WebServerHandler::begin() {
    // Start AsyncWebServer
    server.addHandler(&ws);  // Add WebSocket handler
    server.begin();
    serverStarted = true;
    // Serial.println("[WebServer] Web server started with WebSocket support");
}

void WebServerHandler::end() {
    if (serverStarted) {
        server.end();
        serverStarted = false;
        // Serial.println("[WebServer] Web server stopped");
    }
}

void WebServerHandler::handle() {
    // AsyncWebServer handles requests automatically
    // No need to call handle() for ESP32
}

bool WebServerHandler::isRunning() {
    return serverStarted;
}

AsyncWebServer* WebServerHandler::getServer() {
    return &server;
}



// Web page generation
String WebServerHandler::generateWebPage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>" + String(PROJECT_NAME) + " - Control Panel</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:0;padding:20px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#333;min-height:100vh}";
    html += ".container{max-width:1000px;margin:0 auto;background:rgba(255,255,255,0.95);border-radius:15px;box-shadow:0 20px 40px rgba(0,0,0,0.1);overflow:hidden}";
    html += ".header{background:linear-gradient(135deg,#2c3e50 0%,#34495e 100%);color:white;padding:30px;text-align:center}";
    html += ".header h1{font-size:2.5em;margin:0 0 10px 0;text-shadow:0 2px 4px rgba(0,0,0,0.3)}";
    html += ".content{padding:30px}";
    html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin-bottom:30px}";
    html += ".card{background:white;border-radius:10px;padding:25px;box-shadow:0 5px 15px rgba(0,0,0,0.1);border-left:4px solid #667eea}";
    html += ".card h3{color:#2c3e50;margin:0 0 15px 0;display:flex;align-items:center}";
    html += ".status-indicator{width:12px;height:12px;border-radius:50%;margin-right:10px}";
    html += ".status-connected{background-color:#27ae60}";
    html += ".status-disconnected{background-color:#e74c3c}";
    html += ".info-item{display:flex;justify-content:space-between;margin-bottom:10px;padding:8px 0;border-bottom:1px solid #ecf0f1}";
    html += ".info-item:last-child{border-bottom:none}";
    html += ".info-label{font-weight:600;color:#7f8c8d}";
    html += ".info-value{color:#2c3e50;font-weight:500}";
    html += ".btn{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;border:none;padding:12px 25px;border-radius:25px;cursor:pointer;font-size:14px;font-weight:600;margin:5px;transition:all 0.3s ease;text-decoration:none;display:inline-block}";
    html += ".btn:hover{transform:translateY(-2px);box-shadow:0 5px 15px rgba(102,126,234,0.4)}";
    html += ".btn-danger{background:linear-gradient(135deg,#e74c3c 0%,#c0392b 100%)}";
    html += ".actions{text-align:center;margin-top:30px}";
    html += ".footer{background:#34495e;color:white;text-align:center;padding:20px;font-size:0.9em}";
    html += "@media (max-width:768px){.grid{grid-template-columns:1fr}.header h1{font-size:2em}.content{padding:20px}}";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>" + String(PROJECT_NAME) + "</h1>";
    html += "<p>ESP32 Control Panel - Version " + String(PROJECT_VERSION) + "</p>";
    html += "</div>";
    
    html += "<div class='content'>";
    html += "<div class='grid'>";
    
    // WiFi Status Card
    html += "<div class='card'>";
    html += "<h3><span class='status-indicator' id='wifiStatus'></span>WiFi Status</h3>";
    html += "<div class='info-item'><span class='info-label'>Status:</span><span class='info-value' id='wifiConnected'>Loading...</span></div>";
    html += "<div class='info-item'><span class='info-label'>SSID:</span><span class='info-value' id='wifiSSID'>-</span></div>";
    html += "<div class='info-item'><span class='info-label'>IP Address:</span><span class='info-value' id='wifiIP'>-</span></div>";
    html += "<div class='info-item'><span class='info-label'>Signal:</span><span class='info-value' id='wifiRSSI'>-</span></div>";
    html += "</div>";
    
    // System Info Card
    html += "<div class='card'>";
    html += "<h3><span class='status-indicator status-connected'></span>System Information</h3>";
    html += "<div class='info-item'><span class='info-label'>Status:</span><span class='info-value' id='systemStatus'>Loading...</span></div>";
    html += "<div class='info-item'><span class='info-label'>Uptime:</span><span class='info-value' id='systemUptime'>-</span></div>";
    html += "<div class='info-item'><span class='info-label'>Free Memory:</span><span class='info-value' id='systemMemory'>-</span></div>";
    html += "<div class='info-item'><span class='info-label'>Chip ID:</span><span class='info-value' id='systemChipId'>-</span></div>";
    html += "</div>";
    
    // OTA Card
    html += "<div class='card'>";
    html += "<h3><span class='status-indicator status-connected'></span>OTA Updates</h3>";
    html += "<div class='info-item'><span class='info-label'>Status:</span><span class='info-value' id='otaEnabled'>Ready</span></div>";
    html += "<div class='info-item'><span class='info-label'>Update URL:</span><span class='info-value'><a href='/update' target='_blank' style='color:#667eea'>/update</a></span></div>";
    html += "<div class='info-item'><span class='info-label'>Username:</span><span class='info-value'>" + String(OTA_USERNAME) + "</span></div>";
    html += "</div>";
    
    html += "</div>";
    
    // Actions
    html += "<div class='actions'>";
    html += "<a href='/update' target='_blank' class='btn'>OTA Update</a>";
    html += "<button onclick='refreshData()' class='btn'>Refresh Data</button>";
    html += "<button onclick='restartDevice()' class='btn btn-danger'>Restart Device</button>";
    html += "<button onclick='resetWiFi()' class='btn btn-danger'>Reset WiFi</button>";
    html += "</div>";
    
    html += "</div>";
    
    html += "<div class='footer'>";
    html += "<p>&copy; 2025 " + String(PROJECT_AUTHOR) + " - " + String(PROJECT_NAME) + " v" + String(PROJECT_VERSION) + "</p>";
    html += "</div>";
    
    html += "</div>";
    
    // JavaScript
    html += "<script>";
    html += "function refreshData(){";
    html += "fetch('/api/status').then(r=>r.json()).then(d=>{";
    html += "document.getElementById('wifiStatus').className='status-indicator '+(d.wifi.connected?'status-connected':'status-disconnected');";
    html += "document.getElementById('wifiConnected').textContent=d.wifi.connected?'Connected':'Disconnected';";
    html += "document.getElementById('wifiSSID').textContent=d.wifi.ssid||'-';";
    html += "document.getElementById('wifiIP').textContent=d.wifi.ip||'-';";
    html += "document.getElementById('wifiRSSI').textContent=d.wifi.rssi?d.wifi.rssi+' dBm':'-';";
    html += "document.getElementById('systemStatus').textContent=d.status;";
    html += "document.getElementById('systemUptime').textContent=d.uptime;";
    html += "document.getElementById('systemMemory').textContent=(d.freeHeap/1024).toFixed(1)+' KB';";
    html += "document.getElementById('systemChipId').textContent='0x'+d.chipId.toString(16).toUpperCase();";
    html += "}).catch(e=>console.error('Error:',e));}";
    html += "function restartDevice(){if(confirm('Restart device?')){fetch('/api/restart',{method:'POST'}).then(r=>r.json()).then(d=>alert(d.message));}}";
    html += "function resetWiFi(){if(confirm('Reset WiFi settings? Device will restart.')){fetch('/api/reset',{method:'POST'}).then(r=>r.json()).then(d=>alert(d.message));}}";
    html += "setInterval(refreshData,30000);refreshData();";
    html += "</script>";
    
    html += "</body></html>";
    
    return html;
}

// WebSocket implementation
void WebServerHandler::setupWebSocket() {
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                     AwsEventType type, void *arg, uint8_t *data, size_t len) {
        onWebSocketEvent(server, client, type, arg, data, len);
    });
    
    // Serial.println("[WebServer] WebSocket handler configured");
}

void WebServerHandler::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                                        AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch(type) {
        case WS_EVT_CONNECT:
            // Serial.printf("[WebSocket] Client #%u connected from %s\n", 
            //              client->id(), client->remoteIP().toString().c_str());
            // Send initial sensor data to new client
            {
                DynamicJsonDocument doc(1024);
                doc["type"] = "welcome";
                doc["message"] = "Connected to ESP32 Analog Sensor System";
                doc["timestamp"] = millis();
                String response;
                serializeJson(doc, response);
                client->text(response);
            }
            break;
            
        case WS_EVT_DISCONNECT:
            // Serial.printf("[WebSocket] Client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
            
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WebServerHandler::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char*)data;
        
        // Serial.printf("[WebSocket] Received message: %s\n", message.c_str());
        
        // Parse JSON message
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, message);
        
        if (!error) {
            String command = doc["command"] | "";
            
            if (command == "ping") {
                // Respond to ping
                DynamicJsonDocument response(256);
                response["type"] = "pong";
                response["timestamp"] = millis();
                String responseStr;
                serializeJson(response, responseStr);
                ws.textAll(responseStr);
                
            } else if (command == "get_sensor_data") {
                // Send current sensor data
                broadcastSensorData();
                
            } else if (command == "enable_streaming") {
                bool enable = doc["enable"] | false;
                unsigned long interval = doc["interval"] | 1000;  // Default 1 second
                
                // Enable/disable streaming via analog voltage manager
                extern AnalogVoltageManager analogVoltageMgr;
                analogVoltageMgr.enableStreaming(enable);
                analogVoltageMgr.setStreamInterval(interval);
                
                DynamicJsonDocument response(256);
                response["type"] = "streaming_config";
                response["enabled"] = enable;
                response["interval"] = interval;
                String responseStr;
                serializeJson(response, responseStr);
                ws.textAll(responseStr);
                
                // Serial.printf("[WebSocket] Streaming %s with interval %lu ms\n", 
                //              enable ? "enabled" : "disabled", interval);
            }
        }
    }
}

void WebServerHandler::notifyWebSocketClients(const String& message) {
    if (ws.count() > 0) {
        ws.textAll(message);
    }
}

void WebServerHandler::broadcastToWebSocket(const String& message) {
    notifyWebSocketClients(message);
}

int WebServerHandler::getWebSocketClientCount() {
    return ws.count();
}

void WebServerHandler::broadcastSensorData() {
    extern AnalogVoltageManager analogVoltageMgr;
    if (!analogVoltageMgr.isInitialized()) return;
    
    DynamicJsonDocument doc(2048);
    doc["type"] = "sensor_data";
    doc["timestamp"] = millis();
    
    JsonArray sensors = doc.createNestedArray("sensors");
    AnalogReading* readings = analogVoltageMgr.getAllReadings();
    
    for (int i = 0; i < 3; i++) {
        JsonObject sensor = sensors.createNestedObject();
        sensor["id"] = i;
        sensor["location"] = analogVoltageMgr.getLocation(i);
        sensor["value"] = readings[i].scaledValue;
        sensor["unit"] = analogVoltageMgr.getUnit(i);
        sensor["voltage"] = readings[i].calibratedVoltage;
        sensor["status"] = analogVoltageMgr.getStatusString(i);
        sensor["timestamp"] = readings[i].timestamp;
        sensor["valid"] = readings[i].valid;
        
        // Health data
        sensor["health_score"] = analogVoltageMgr.getHealthScore(i);
        sensor["is_dead"] = analogVoltageMgr.isDeadSensor(i);
        sensor["is_stuck"] = analogVoltageMgr.isStuckSensor(i);
        
        // Alarm status
        sensor["is_alarm"] = (analogVoltageMgr.isLowValue(i) || analogVoltageMgr.isHighValue(i));
        sensor["low_threshold"] = analogVoltageMgr.getLowThreshold(i);
        sensor["high_threshold"] = analogVoltageMgr.getHighThreshold(i);
    }
    
    String message;
    serializeJson(doc, message);
    notifyWebSocketClients(message);
}
