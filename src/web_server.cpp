#include "web_server.h"
#include "ota_handler.h"
#include "system_manager.h"
#include "sd_manager.h"
#include "ntp_manager.h"
#include "analog_voltage_manager.h"
#include "analytics_manager.h"
#include "remote_diagnostics.h"
#include "webhook_handler.h"

// Global instance
WebServerHandler webServer;

WebServerHandler::WebServerHandler() 
    : server(WEB_SERVER_PORT), ws("/ws")
{
    serverStarted = false;
}

bool WebServerHandler::initialize() {
    // Serial.println("[WebServer] Initializing web server...");
    
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
    
    // Analog Voltage API endpoint
    server.on("/api/analog-voltage", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        
        doc["initialized"] = analogVoltageMgr.isInitialized();
        doc["total_readings"] = analogVoltageMgr.getTotalReadings();
        doc["alarm_status"] = analogVoltageMgr.getAlarmStatus();
        doc["has_errors"] = analogVoltageMgr.hasErrors();
        
        for (int i = 0; i < 3; i++) {
            AnalogReading reading = analogVoltageMgr.getReading(i);
            String sensorKey = "sensor_" + String(i);
            
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
            String sensorKey = "sensor_" + String(i);
            
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
            String sensorKey = "sensor_" + String(i);
            
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
        request->send(200, "application/json", webhookHandler.getWebhooksJSON());
    });
    
    server.on("/api/webhooks", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern WebhookHandler webhookHandler;
        
        String url = "";
        String secret = "";
        String authToken = "";
        
        if (request->hasParam("url", true)) {
            url = request->getParam("url", true)->value();
        }
        if (request->hasParam("secret", true)) {
            secret = request->getParam("secret", true)->value();
        }
        if (request->hasParam("auth_token", true)) {
            authToken = request->getParam("auth_token", true)->value();
        }
        
        if (url.length() == 0) {
            request->send(400, "application/json", "{\"error\":\"URL is required\"}");
            return;
        }
        
        String webhookId = webhookHandler.addWebhook(url, secret, authToken);
        if (webhookId.length() > 0) {
            DynamicJsonDocument doc(256);
            doc["success"] = true;
            doc["webhook_id"] = webhookId;
            doc["message"] = "Webhook added successfully";
            
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        } else {
            request->send(400, "application/json", "{\"error\":\"Failed to add webhook\"}");
        }
    });
    
    server.on("/api/webhooks/test", HTTP_POST, [this](AsyncWebServerRequest *request) {
        extern WebhookHandler webhookHandler;
        
        String webhookId = "";
        if (request->hasParam("webhook_id", true)) {
            webhookId = request->getParam("webhook_id", true)->value();
        }
        
        bool success = webhookHandler.testWebhook(webhookId);
        
        DynamicJsonDocument doc(256);
        doc["success"] = success;
        doc["message"] = success ? "Test webhook sent" : "Failed to send test webhook";
        
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
