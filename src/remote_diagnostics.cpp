#include "remote_diagnostics.h"
#include "sd_manager.h"
#include "wifi_manager.h"
#include "analog_voltage_manager.h"
#include <WiFi.h>
#include <ESP.h>
#include <esp_system.h>
#include <esp_wifi.h>

// Global instance
RemoteDiagnostics remoteDiag;

RemoteDiagnostics::RemoteDiagnostics() :
    alertCount(0),
    alertIndex(0),
    commandCount(0),
    commandIndex(0),
    diagnosticsEnabled(true),
    diagnosticInterval(DEFAULT_DIAGNOSTIC_INTERVAL),
    lastDiagnosticTime(0),
    lastSystemCheck(0),
    lastNetworkCheck(0),
    lastStorageCheck(0),
    remoteCommandsEnabled(true),
    initialized(false),
    loopCounter(0),
    lastLoopTime(0),
    maxLoopTime(0),
    avgLoopTime(0.0) {
    
    // Initialize metrics
    systemMetrics = {0, 0, 0, 0, 0.0, 0.0, 0, 0.0, 0, 0};
    networkDiag = {false, "", 0, "", "", "", 0, 0, 0, 0, 0, 0.0, 0, ""};
    storageDiag = {false, 0, 0, 0, 0.0, 0, 0, 0, 0, "", 0.0};
    
    // Initialize sensor diagnostics
    for (int i = 0; i < 3; i++) {
        sensorDiag[i] = {i, false, false, 0, 0, 0, 0.0, 0.0, 0.0, "", 0, false, 0.0};
    }
}

bool RemoteDiagnostics::begin() {
    Serial.println("[RemoteDiag] Initializing remote diagnostics...");
    
    // Clear alerts and commands
    alertCount = 0;
    commandCount = 0;
    
    // Initialize timing
    lastDiagnosticTime = millis();
    lastSystemCheck = lastDiagnosticTime;
    lastNetworkCheck = lastDiagnosticTime;
    lastStorageCheck = lastDiagnosticTime;
    
    // Get initial chip temperature
    systemMetrics.chipTemperature = temperatureRead();
    
    // Add startup alert
    addAlert(DIAG_INFO, "system", "Remote diagnostics initialized", "System is ready for monitoring");
    
    initialized = true;
    Serial.println("[RemoteDiag] Remote diagnostics initialized successfully");
    return true;
}

void RemoteDiagnostics::setDiagnosticInterval(unsigned long interval) {
    diagnosticInterval = max(interval, 10000UL);  // Minimum 10 seconds
    
    if (DEBUG_ENABLED) {
        Serial.printf("[RemoteDiag] Diagnostic interval set to %lu ms\n", diagnosticInterval);
    }
}

void RemoteDiagnostics::enableRemoteCommands(bool enable) {
    remoteCommandsEnabled = enable;
    
    if (DEBUG_ENABLED) {
        Serial.printf("[RemoteDiag] Remote commands %s\n", enable ? "enabled" : "disabled");
    }
}

void RemoteDiagnostics::handle() {
    if (!initialized || !diagnosticsEnabled) return;
    
    unsigned long currentTime = millis();
    
    // Record loop performance
    if (lastLoopTime > 0) {
        unsigned long loopTime = currentTime - lastLoopTime;
        recordLoopTime(loopTime);
    }
    lastLoopTime = currentTime;
    
    // System checks at different intervals
    if (currentTime - lastSystemCheck >= SYSTEM_CHECK_INTERVAL) {
        updateSystemMetrics();
        checkSystemHealth();
        lastSystemCheck = currentTime;
    }
    
    if (currentTime - lastNetworkCheck >= NETWORK_CHECK_INTERVAL) {
        updateNetworkDiagnostics();
        checkNetworkHealth();
        lastNetworkCheck = currentTime;
    }
    
    if (currentTime - lastStorageCheck >= STORAGE_CHECK_INTERVAL) {
        updateStorageDiagnostics();
        checkStorageHealth();
        lastStorageCheck = currentTime;
    }
    
    // Full diagnostics run
    if (currentTime - lastDiagnosticTime >= diagnosticInterval) {
        runDiagnostics();
        lastDiagnosticTime = currentTime;
    }
    
    // Process remote commands
    if (remoteCommandsEnabled) {
        processRemoteCommands();
    }
}

void RemoteDiagnostics::runDiagnostics() {
    updateSensorDiagnostics();
    
    // Check sensor health
    for (int i = 0; i < 3; i++) {
        checkSensorHealth(i);
    }
    
    logDiagnostics();
    
    if (DEBUG_ENABLED) {
        Serial.printf("[RemoteDiag] Diagnostic cycle completed. Health score: %.1f%%\n", getOverallHealthScore());
    }
}

void RemoteDiagnostics::updateSystemMetrics() {
    systemMetrics.uptime = millis();
    systemMetrics.freeHeap = ESP.getFreeHeap();
    systemMetrics.totalHeap = ESP.getHeapSize();
    systemMetrics.minFreeHeap = ESP.getMinFreeHeap();
    systemMetrics.memoryUsage = ((float)(systemMetrics.totalHeap - systemMetrics.freeHeap) / systemMetrics.totalHeap) * 100.0;
    systemMetrics.cpuUsage = calculateCPUUsage();
    systemMetrics.chipTemperature = temperatureRead();
    systemMetrics.resetReason = esp_reset_reason();
}

void RemoteDiagnostics::updateNetworkDiagnostics() {
    extern WiFiManagerHandler wifiMgr;
    
    networkDiag.wifiConnected = wifiMgr.isConnected();
    
    if (networkDiag.wifiConnected) {
        networkDiag.ssid = wifiMgr.getSSID();
        networkDiag.rssi = wifiMgr.getRSSI();
        networkDiag.ip = wifiMgr.getIP();
        networkDiag.gateway = WiFi.gatewayIP().toString();
        networkDiag.dns = WiFi.dnsIP().toString();
        networkDiag.macAddress = WiFi.macAddress();
        
        // Simple ping test
        networkDiag.pingLatency = pingTest();
        
        // Update connection time if this is a new connection
        static bool wasConnected = false;
        if (!wasConnected) {
            networkDiag.connectionTime = millis();
        }
        wasConnected = true;
    } else {
        // Count disconnections
        static bool wasConnected = true;
        if (wasConnected) {
            networkDiag.disconnectionCount++;
        }
        wasConnected = false;
        
        networkDiag.pingLatency = -1;
    }
}

void RemoteDiagnostics::updateStorageDiagnostics() {
    extern SDManager sdMgr;
    
    storageDiag.sdCardMounted = sdMgr.isMounted();
    
    if (storageDiag.sdCardMounted) {
        // Get SD card info (simplified - would need proper implementation)
        storageDiag.totalSpace = 1024 * 1024 * 1024;  // Placeholder: 1GB
        storageDiag.usedSpace = storageDiag.totalSpace * 0.1;  // Placeholder: 10% used
        storageDiag.freeSpace = storageDiag.totalSpace - storageDiag.usedSpace;
        storageDiag.usagePercentage = (float)storageDiag.usedSpace / storageDiag.totalSpace * 100.0;
        storageDiag.cardType = "SDHC";  // Placeholder
        storageDiag.cardSpeed = 25.0;   // Placeholder: 25 MB/s
        
        // Update last write time when SD operations occur
        storageDiag.lastWriteTime = millis();  // Simplified - use current time
    }
}

void RemoteDiagnostics::updateSensorDiagnostics() {
    extern AnalogVoltageManager analogVoltageMgr;
    
    for (int i = 0; i < 3; i++) {
        sensorDiag[i].sensorId = i;
        sensorDiag[i].isOnline = analogVoltageMgr.isSensorEnabled(i);
        sensorDiag[i].isResponding = (analogVoltageMgr.getSensorStatus(i) == ANALOG_SENSOR_OK);
        
        AnalogReading reading = analogVoltageMgr.getReading(i);
        sensorDiag[i].lastReadTime = reading.timestamp;
        
        // Calculate error rates (simplified)
        unsigned long totalReads = analogVoltageMgr.getTotalReadings();
        unsigned long errors = analogVoltageMgr.getErrorCount(i);
        
        if (totalReads > 0) {
            sensorDiag[i].errorRate = ((float)errors / totalReads) * 100.0;
            sensorDiag[i].successfulReads = totalReads - errors;
            sensorDiag[i].failedReads = errors;
        }
        
        // Signal quality based on sensor status and health
        if (sensorDiag[i].isResponding) {
            float healthScore = analogVoltageMgr.getHealthScore(i);
            sensorDiag[i].signalQuality = healthScore;
        } else {
            sensorDiag[i].signalQuality = 0.0;
        }
        
        // Check if calibration is needed
        sensorDiag[i].needsCalibration = !analogVoltageMgr.isCalibrated(i);
        
        // Calculate average response time (placeholder)
        sensorDiag[i].avgResponseTime = 5.0;  // ms
    }
}

void RemoteDiagnostics::checkSystemHealth() {
    // Memory usage check
    if (systemMetrics.memoryUsage > 90.0) {
        addAlert(DIAG_CRITICAL, "system", "High memory usage: " + String(systemMetrics.memoryUsage, 1) + "%", 
                "Consider restarting the system or reducing data collection frequency");
    } else if (systemMetrics.memoryUsage > 75.0) {
        addAlert(DIAG_WARNING, "system", "Elevated memory usage: " + String(systemMetrics.memoryUsage, 1) + "%", 
                "Monitor memory usage closely");
    }
    
    // CPU usage check
    if (systemMetrics.cpuUsage > 95.0) {
        addAlert(DIAG_CRITICAL, "system", "High CPU usage: " + String(systemMetrics.cpuUsage, 1) + "%", 
                "System may be overloaded");
    }
    
    // Temperature check
    if (systemMetrics.chipTemperature > 80.0) {
        addAlert(DIAG_WARNING, "system", "High chip temperature: " + String(systemMetrics.chipTemperature, 1) + "°C", 
                "Check ventilation and ambient temperature");
    }
    
    // Free heap check
    if (systemMetrics.freeHeap < 10000) {
        addAlert(DIAG_CRITICAL, "system", "Low free heap: " + String(systemMetrics.freeHeap) + " bytes", 
                "System may become unstable");
    }
}

void RemoteDiagnostics::checkNetworkHealth() {
    if (!networkDiag.wifiConnected) {
        addAlert(DIAG_ERROR, "network", "WiFi disconnected", "Check network settings and signal strength");
    } else {
        // Signal strength check
        if (networkDiag.rssi < -80) {
            addAlert(DIAG_WARNING, "network", "Weak WiFi signal: " + String(networkDiag.rssi) + " dBm", 
                    "Consider improving antenna position or using WiFi extender");
        }
        
        // Ping latency check
        if (networkDiag.pingLatency > 1000) {
            addAlert(DIAG_WARNING, "network", "High network latency: " + String(networkDiag.pingLatency) + " ms", 
                    "Check network congestion");
        }
    }
}

void RemoteDiagnostics::checkStorageHealth() {
    if (!storageDiag.sdCardMounted) {
        addAlert(DIAG_ERROR, "storage", "SD card not mounted", "Check SD card connection and format");
    } else {
        // Storage usage check
        if (storageDiag.usagePercentage > 95.0) {
            addAlert(DIAG_CRITICAL, "storage", "SD card almost full: " + String(storageDiag.usagePercentage, 1) + "%", 
                    "Clean up old files or replace with larger SD card");
        } else if (storageDiag.usagePercentage > 85.0) {
            addAlert(DIAG_WARNING, "storage", "SD card usage high: " + String(storageDiag.usagePercentage, 1) + "%", 
                    "Consider cleaning up old files");
        }
    }
}

void RemoteDiagnostics::checkSensorHealth(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    SensorDiagnostics& sensor = sensorDiag[sensorIndex];
    
    if (!sensor.isOnline) {
        addAlert(DIAG_WARNING, "sensor", "Sensor " + String(sensorIndex) + " is offline", 
                "Check sensor configuration and enable if needed");
    } else if (!sensor.isResponding) {
        addAlert(DIAG_ERROR, "sensor", "Sensor " + String(sensorIndex) + " not responding", 
                "Check sensor wiring and connections");
    } else {
        // Error rate check
        if (sensor.errorRate > 10.0) {
            addAlert(DIAG_WARNING, "sensor", "Sensor " + String(sensorIndex) + " high error rate: " + String(sensor.errorRate, 1) + "%", 
                    "Check sensor connections and calibration");
        }
        
        // Signal quality check
        if (sensor.signalQuality < 50.0) {
            addAlert(DIAG_WARNING, "sensor", "Sensor " + String(sensorIndex) + " poor signal quality: " + String(sensor.signalQuality, 1) + "%", 
                    "Check sensor calibration and environment");
        }
        
        // Calibration check
        if (sensor.needsCalibration) {
            addAlert(DIAG_INFO, "sensor", "Sensor " + String(sensorIndex) + " needs calibration", 
                    "Run sensor calibration procedure");
        }
    }
}

void RemoteDiagnostics::addAlert(DiagnosticLevel level, const String& category, const String& message, const String& recommendation) {
    // Check if similar alert already exists
    for (int i = 0; i < alertCount; i++) {
        if (alerts[i].category == category && alerts[i].message == message && !alerts[i].acknowledged) {
            // Update existing alert timestamp
            alerts[i].timestamp = millis();
            return;
        }
    }
    
    // Add new alert
    if (alertCount < MAX_ALERTS) {
        alerts[alertCount] = {level, category, message, recommendation, millis(), false, "RemoteDiagnostics"};
        alertCount++;
    } else {
        // Replace oldest alert
        alerts[alertIndex] = {level, category, message, recommendation, millis(), false, "RemoteDiagnostics"};
        alertIndex = (alertIndex + 1) % MAX_ALERTS;
    }
    
    if (DEBUG_ENABLED) {
        String levelStr = (level == DIAG_CRITICAL) ? "CRITICAL" : 
                         (level == DIAG_ERROR) ? "ERROR" : 
                         (level == DIAG_WARNING) ? "WARNING" : "INFO";
        Serial.printf("[RemoteDiag] %s [%s]: %s\n", levelStr.c_str(), category.c_str(), message.c_str());
    }
}

float RemoteDiagnostics::calculateCPUUsage() {
    // Simplified CPU usage calculation based on loop timing
    if (avgLoopTime > 0) {
        return min(100.0f, (avgLoopTime / 100.0f) * 100.0f);  // Very rough estimate
    }
    return 0.0;
}

int RemoteDiagnostics::pingTest(const String& host) {
    // Simplified ping test - would need proper implementation
    return networkDiag.wifiConnected ? 25 : -1;  // Placeholder
}

void RemoteDiagnostics::recordLoopTime(unsigned long loopTime) {
    loopCounter++;
    
    if (loopTime > maxLoopTime) {
        maxLoopTime = loopTime;
    }
    
    // Calculate running average
    avgLoopTime = (avgLoopTime * 0.9) + (loopTime * 0.1);
}

void RemoteDiagnostics::logDiagnostics() {
    extern SDManager sdMgr;
    if (!sdMgr.isMounted()) return;
    
    String diagLog = "DIAGNOSTICS,";
    diagLog += String(systemMetrics.memoryUsage, 1) + ",";
    diagLog += String(systemMetrics.cpuUsage, 1) + ",";
    diagLog += String(systemMetrics.chipTemperature, 1) + ",";
    diagLog += String(networkDiag.rssi) + ",";
    diagLog += String(storageDiag.usagePercentage, 1) + ",";
    diagLog += String(alertCount);
    
    sdMgr.logDataWithTimestamp(diagLog);
}

// Public access methods
SystemMetrics RemoteDiagnostics::getSystemMetrics() {
    return systemMetrics;
}

String RemoteDiagnostics::getAllDiagnosticsJSON() {
    DynamicJsonDocument doc(4096);
    
    // System metrics
    JsonObject system = doc.createNestedObject("system");
    system["uptime"] = systemMetrics.uptime;
    system["free_heap"] = systemMetrics.freeHeap;
    system["memory_usage"] = systemMetrics.memoryUsage;
    system["cpu_usage"] = systemMetrics.cpuUsage;
    system["temperature"] = systemMetrics.chipTemperature;
    system["reset_reason"] = systemMetrics.resetReason;
    
    // Network diagnostics
    JsonObject network = doc.createNestedObject("network");
    network["connected"] = networkDiag.wifiConnected;
    network["ssid"] = networkDiag.ssid;
    network["rssi"] = networkDiag.rssi;
    network["ip"] = networkDiag.ip;
    network["ping_latency"] = networkDiag.pingLatency;
    
    // Storage diagnostics
    JsonObject storage = doc.createNestedObject("storage");
    storage["mounted"] = storageDiag.sdCardMounted;
    storage["usage_percent"] = storageDiag.usagePercentage;
    storage["free_space"] = storageDiag.freeSpace;
    storage["card_type"] = storageDiag.cardType;
    
    // Sensor diagnostics
    JsonArray sensors = doc.createNestedArray("sensors");
    for (int i = 0; i < 3; i++) {
        JsonObject sensor = sensors.createNestedObject();
        sensor["id"] = i;
        sensor["online"] = sensorDiag[i].isOnline;
        sensor["responding"] = sensorDiag[i].isResponding;
        sensor["error_rate"] = sensorDiag[i].errorRate;
        sensor["signal_quality"] = sensorDiag[i].signalQuality;
        sensor["needs_calibration"] = sensorDiag[i].needsCalibration;
    }
    
    // Performance metrics
    JsonObject performance = doc.createNestedObject("performance");
    performance["avg_loop_time"] = avgLoopTime;
    performance["max_loop_time"] = maxLoopTime;
    performance["loop_counter"] = loopCounter;
    
    // Overall health
    doc["health_score"] = getOverallHealthScore();
    doc["alert_count"] = alertCount;
    
    String json;
    serializeJson(doc, json);
    return json;
}

String RemoteDiagnostics::getAlertsJSON() {
    DynamicJsonDocument doc(2048);
    
    JsonArray alertsArray = doc.createNestedArray("alerts");
    
    for (int i = 0; i < alertCount; i++) {
        JsonObject alert = alertsArray.createNestedObject();
        alert["level"] = alerts[i].level;
        alert["category"] = alerts[i].category;
        alert["message"] = alerts[i].message;
        alert["recommendation"] = alerts[i].recommendation;
        alert["timestamp"] = alerts[i].timestamp;
        alert["acknowledged"] = alerts[i].acknowledged;
    }
    
    doc["total_alerts"] = alertCount;
    
    String json;
    serializeJson(doc, json);
    return json;
}

float RemoteDiagnostics::getOverallHealthScore() {
    float score = 100.0;
    
    // Deduct points for various issues
    if (systemMetrics.memoryUsage > 90) score -= 30;
    else if (systemMetrics.memoryUsage > 75) score -= 15;
    
    if (!networkDiag.wifiConnected) score -= 25;
    else if (networkDiag.rssi < -80) score -= 10;
    
    if (!storageDiag.sdCardMounted) score -= 20;
    else if (storageDiag.usagePercentage > 95) score -= 15;
    
    // Check sensors
    for (int i = 0; i < 3; i++) {
        if (!sensorDiag[i].isOnline) score -= 10;
        else if (!sensorDiag[i].isResponding) score -= 15;
        else if (sensorDiag[i].errorRate > 10) score -= 5;
    }
    
    return max(0.0f, score);
}

void RemoteDiagnostics::clearAlerts() {
    alertCount = 0;
    alertIndex = 0;
}

bool RemoteDiagnostics::isInitialized() const {
    return initialized;
}

String RemoteDiagnostics::getStatus() {
    DynamicJsonDocument doc(512);
    
    doc["initialized"] = initialized;
    doc["enabled"] = diagnosticsEnabled;
    doc["interval"] = diagnosticInterval;
    doc["remote_commands"] = remoteCommandsEnabled;
    doc["last_diagnostic"] = lastDiagnosticTime;
    doc["health_score"] = getOverallHealthScore();
    
    String status;
    serializeJson(doc, status);
    return status;
}

// Placeholder implementations for other methods
void RemoteDiagnostics::processRemoteCommands() {
    // Implementation for processing remote commands
}

void RemoteDiagnostics::executeCommand(RemoteCommand& cmd) {
    // Implementation for executing specific commands
}

void RemoteDiagnostics::addRemoteCommand(const String& command, const String& parameters, const String& requestId) {
    // Implementation for adding remote commands
}

String RemoteDiagnostics::performHealthCheck() {
    return getAllDiagnosticsJSON();
}

bool RemoteDiagnostics::isSystemHealthy() {
    // Check for critical alerts
    for (int i = 0; i < alertCount; i++) {
        if (alerts[i].level == DIAG_CRITICAL) {
            return false;
        }
    }
    
    // Check system metrics
    if (systemMetrics.freeHeap < 10000) return false;  // Less than 10KB free
    if (systemMetrics.cpuUsage > 90.0) return false;   // CPU usage over 90%
    if (systemMetrics.chipTemperature > 80.0) return false; // Temperature over 80°C
    
    // Check network connectivity
    if (!networkDiag.wifiConnected) return false;
    
    // Check storage
    if (storageDiag.freeSpace < 1000) return false;    // Less than 1KB free
    
    return true;
}

int RemoteDiagnostics::getAlertCount() {
    return alertCount;
}

unsigned long RemoteDiagnostics::getLastDiagnosticTime() {
    return lastDiagnosticTime;
}