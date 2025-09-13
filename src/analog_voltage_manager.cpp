#include "analog_voltage_manager.h"
#include "sd_manager.h"
#include "ntp_manager.h"
#include "web_server.h"
#include "webhook_handler.h"

// Global instance
AnalogVoltageManager analogVoltageMgr;

AnalogVoltageManager::AnalogVoltageManager() : 
    lastReadTime(0),
    readInterval(1000),  // Default 1 second interval
    lastLogTime(0),
    logInterval(60000),  // Default 1 minute logging interval
    lastHealthCheck(0),
    lastStreamTime(0),
    streamInterval(DEFAULT_STREAM_INTERVAL),
    streamingEnabled(true),
    simulationEnabled(false),
    initialized(false),
    totalReadings(0),
    loggingEnabled(true) {
    
    // Initialize sensor configurations with default values
    sensors[0] = {"AI1 Sensor", "%", "AI1_SENS_001", "Generic", "Unknown", "N/A", "", "AI1 analog sensor", "", "", 
                  0.0, 100.0, 20.0, 80.0, true, 0.3, true, 20.0, 0.0, 1.0, false, "", "", 2, "", true};
    sensors[1] = {"AI2 Sensor", "%", "AI2_SENS_002", "Generic", "Unknown", "N/A", "", "AI2 analog sensor", "", "",
                  0.0, 100.0, 20.0, 80.0, true, 0.3, true, 20.0, 0.0, 1.0, false, "", "", 2, "", true};
    sensors[2] = {"AI3 Sensor", "%", "AI3_SENS_003", "Generic", "Unknown", "N/A", "", "AI3 analog sensor", "", "",
                  0.0, 100.0, 20.0, 80.0, true, 0.3, true, 20.0, 0.0, 1.0, false, "", "", 2, "", true};
    
    // Initialize simulation variables
    for (int i = 0; i < 3; i++) {
        sensorSimulated[i] = false;
        simulationMode[i] = "fixed";
        simulationValue[i] = 50.0;      // Default 50% for demo
        simulationAmplitude[i] = 10.0;   // ±10% amplitude
        simulationFrequency[i] = 0.1;    // 0.1 Hz (10 second period)
        simulationStartTime[i] = 0;
    }
    
    // Initialize readings and filters
    for (int i = 0; i < 3; i++) {
        readings[i] = {0, 0.0, 0.0, 0.0, ANALOG_SENSOR_ERROR, 0, false};
        errorCount[i] = 0;
        emaFilteredValue[i] = 0.0;
        lastValidReading[i] = 0.0;
        filterInitTime[i] = 0;
        
        // Initialize health data
        healthData[i] = {0.0, 0.0, 0.0, {0}, 0, 0, 0, 0, false, false, false, 100.0};
    }
}

bool AnalogVoltageManager::begin() {
    // if (DEBUG_ENABLED) {
    //     // Serial.println("[AV] Initializing Analog Voltage Manager...");
    // }
    
    // Set analog resolution
    analogReadResolution(12);  // 12-bit resolution (0-4095)
    
    // Initial sensor readings
    for (int i = 0; i < 3; i++) {
        if (sensors[i].enabled) {
            updateSensorStatus(i);
        }
    }
    
    initialized = true;
    lastReadTime = millis();
    
    // if (DEBUG_ENABLED) {
    //     // Serial.println("[AV] Analog Voltage Manager initialized successfully");
    //     // Serial.printf("[AV] Configured sensors: AI1 (GPIO35), AI2 (GPIO34), AI3 (GPIO36)\n");
    // }
    
    return true;
}

void AnalogVoltageManager::configureSensor(int sensorIndex, const String& location, const String& unit,
                                           float minValue, float maxValue,
                                           float lowThreshold, float highThreshold) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    sensors[sensorIndex].location = location;
    sensors[sensorIndex].unit = unit;
    sensors[sensorIndex].minValue = minValue;
    sensors[sensorIndex].maxValue = maxValue;
    sensors[sensorIndex].lowThreshold = lowThreshold;
    sensors[sensorIndex].highThreshold = highThreshold;
    
    // Reset filter when reconfiguring
    resetFilter(sensorIndex);
    
    // if (DEBUG_ENABLED) {
    //     // Serial.printf("[AV] Sensor %d configured: %s (%.1f-%.1f %s)\n", 
    //                  sensorIndex, location.c_str(), minValue, maxValue, unit.c_str());
    // }
}

void AnalogVoltageManager::setReadInterval(unsigned long interval) {
    readInterval = max(interval, 100UL);  // Minimum 100ms
}

void AnalogVoltageManager::setLogInterval(unsigned long interval) {
    logInterval = max(interval, 10000UL);  // Minimum 10 seconds
}

void AnalogVoltageManager::enableLogging(bool enable) {
    loggingEnabled = enable;
    
    // if (DEBUG_ENABLED) {
    //     // Serial.printf("[AV] Data logging %s\n", enable ? "enabled" : "disabled");
    // }
}

void AnalogVoltageManager::enableSensor(int sensorIndex, bool enable) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    sensors[sensorIndex].enabled = enable;
    
    // if (DEBUG_ENABLED) {
    //     // Serial.printf("[AV] Sensor %d %s\n", sensorIndex, enable ? "enabled" : "disabled");
    // }
}

void AnalogVoltageManager::handle() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // Check if it's time to read sensors
    if (currentTime - lastReadTime >= readInterval) {
        requestReading();
    }
    
    // Periodic health evaluation
    if (currentTime - lastHealthCheck >= HEALTH_CHECK_INTERVAL) {
        for (int i = 0; i < 3; i++) {
            if (sensors[i].enabled) {
                evaluateSensorHealth(i);
            }
        }
        lastHealthCheck = currentTime;
    }
    
    // Check if it's time to log data
    if (loggingEnabled && (currentTime - lastLogTime >= logInterval)) {
        logSensorData();
        lastLogTime = currentTime;
    }
    
    // Real-time streaming to WebSocket clients
    if (streamingEnabled && (currentTime - lastStreamTime >= streamInterval)) {
        generateDataStreamEvent();
        lastStreamTime = currentTime;
    }
}

bool AnalogVoltageManager::isReadingReady() {
    return initialized && (millis() - lastReadTime >= readInterval);
}

void AnalogVoltageManager::requestReading() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    for (int i = 0; i < 3; i++) {
        if (!sensors[i].enabled) continue;
        
        float calibratedVoltage;
        int rawADC;
        
        // Check if sensor is in simulation mode
        if (isSensorSimulated(i)) {
            // Generate simulated data
            float simulatedValue = generateSimulatedReading(i);
            
            // Convert simulated scaled value back to voltage (reverse of voltageToScaledValue)
            float valueRange = sensors[i].maxValue - sensors[i].minValue;
            float normalizedValue = (simulatedValue - sensors[i].minValue) / valueRange;
            calibratedVoltage = normalizedValue * 10.0;  // Scale to 0-10V range
            
            // Generate corresponding ADC value for consistency
            rawADC = (int)(calibratedVoltage * 4095.0 / 3.3);  // Approximate reverse conversion
            
            if (DEBUG_ENABLED && totalReadings % 50 == 0) {
                // // Serial.printf("[AV] Sensor %d SIMULATED: %.2f %s (%.3fV)\n", 
                //              i, simulatedValue, sensors[i].unit.c_str(), calibratedVoltage);
            }
            
        } else {
            // Read real analog pin with advanced filtering
            int pin = (i == 0) ? AI1_PIN : (i == 1) ? AI2_PIN : AI3_PIN;
            rawADC = readAnalogSmoothed(pin);
            
            // Convert to voltage and apply advanced filtering
            calibratedVoltage = processAnalogReading(i, rawADC);
        }
        
        // Apply calibration if configured (both real and simulated)
        if (sensors[i].calibrated) {
            calibratedVoltage = (calibratedVoltage + sensors[i].offsetCorrection) * sensors[i].gainCorrection;
        }
        
        // Update sensor health monitoring
        updateSensorHealth(i, calibratedVoltage);
        
        // Update reading
        readings[i].rawADC = rawADC;
        readings[i].voltage = rawADC / 4095.0 * 3.3;  // Raw ESP32 voltage
        readings[i].calibratedVoltage = calibratedVoltage;
        readings[i].scaledValue = voltageToScaledValue(i, calibratedVoltage);
        readings[i].timestamp = currentTime;
        readings[i].status = validateReading(i, calibratedVoltage);
        readings[i].valid = (readings[i].status == ANALOG_SENSOR_OK);
        
        updateSensorStatus(i);
        
        // Check for alarm conditions and notify if needed
        if (sensors[i].alarmEnabled) {
            if (isLowValue(i)) {
                triggerAlarmNotification(i, "LOW_ALARM");
            } else if (isHighValue(i)) {
                triggerAlarmNotification(i, "HIGH_ALARM");
            }
            
            // Health-based alarms
            if (healthData[i].isDead) {
                triggerAlarmNotification(i, "SENSOR_DEAD");
            } else if (healthData[i].isStuck) {
                triggerAlarmNotification(i, "SENSOR_STUCK");
            }
        }
    }
    
    totalReadings++;
}

float AnalogVoltageManager::convert010V(int adc) {
    // Use the calibration function from the sample
    float Vadc = adc / 4095.0 * 3.3;
    float Volt = Vadc / 3.3 * 10.0;
    float Vcal;

    if (Vadc == 0.0) {
        Vcal = 0.0;
    } else if (Vadc > 0.01 && Vadc <= 0.96) {
        Vcal = 1.0345 * Volt + 0.2897;
    } else if (Vadc > 0.96 && Vadc <= 1.52) {
        Vcal = 1.0029 * Volt + 0.3814;
    } else if (Vadc > 1.52) {
        Vcal = 0.932 * Volt + 0.7083;
    } else if (Vadc == 3.3) {
        Vcal = 10.0;
    } else {
        Vcal = Volt; // Fallback to uncalibrated
    }

    return Vcal;
}

int AnalogVoltageManager::readAnalogSmoothed(int pin) {
    int sum = 0;
    
    // Take multiple samples for smoothing
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(pin);
        delayMicroseconds(100);  // Small delay between samples
    }
    
    return sum / NUM_SAMPLES;
}

float AnalogVoltageManager::processAnalogReading(int sensorIndex, int rawADC) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    
    // Convert ADC to voltage
    float voltage = convert010V(rawADC);
    
    // Apply outlier detection if enabled
    if (sensors[sensorIndex].outlierDetection && isOutlier(sensorIndex, voltage)) {
        if (DEBUG_ENABLED) {
            // // Serial.printf("[AV] Outlier detected on sensor %d: %.2fV, using last valid: %.2fV\n", 
            //              sensorIndex, voltage, lastValidReading[sensorIndex]);
        }
        voltage = lastValidReading[sensorIndex]; // Use last valid reading
    } else {
        lastValidReading[sensorIndex] = voltage; // Update last valid reading
    }
    
    // Apply EMA filter
    voltage = applyEMAFilter(sensorIndex, voltage);
    
    return voltage;
}

float AnalogVoltageManager::applyEMAFilter(int sensorIndex, float newValue) {
    if (sensorIndex < 0 || sensorIndex >= 3) return newValue;
    
    // Initialize filter on first reading
    if (filterInitTime[sensorIndex] == 0) {
        emaFilteredValue[sensorIndex] = newValue;
        filterInitTime[sensorIndex] = millis();
        return newValue;
    }
    
    // Apply EMA: filtered = α * new + (1-α) * filtered
    float alpha = sensors[sensorIndex].smoothingFactor;
    emaFilteredValue[sensorIndex] = alpha * newValue + (1.0 - alpha) * emaFilteredValue[sensorIndex];
    
    return emaFilteredValue[sensorIndex];
}

bool AnalogVoltageManager::isOutlier(int sensorIndex, float value) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    
    // Skip outlier detection for first few readings
    if (filterInitTime[sensorIndex] == 0 || lastValidReading[sensorIndex] == 0.0) {
        return false;
    }
    
    // Calculate percentage deviation from last valid reading
    float deviation = abs(value - lastValidReading[sensorIndex]) / lastValidReading[sensorIndex] * 100.0;
    
    return deviation > sensors[sensorIndex].outlierThreshold;
}

AnalogSensorStatus AnalogVoltageManager::validateReading(int sensorIndex, float voltage) {
    if (sensorIndex < 0 || sensorIndex >= 3) return ANALOG_SENSOR_ERROR;
    
    // Check for dead sensor (multiple criteria)
    if (detectDeadSensor(sensorIndex, voltage)) {
        healthData[sensorIndex].isDead = true;
        return ANALOG_SENSOR_DISCONNECTED;
    }
    
    // Check for stuck sensor
    if (detectStuckSensor(sensorIndex, voltage)) {
        healthData[sensorIndex].isStuck = true;
        // Stuck sensor is still functional, just suspicious
    }
    
    // Check for out of range (above 10V is unusual)
    if (voltage > 10.5) {
        return ANALOG_SENSOR_OUT_OF_RANGE;
    }
    
    // Additional validation based on sensor configuration
    float scaledValue = voltageToScaledValue(sensorIndex, voltage);
    if (scaledValue < sensors[sensorIndex].minValue - 5.0 || 
        scaledValue > sensors[sensorIndex].maxValue + 5.0) {
        return ANALOG_SENSOR_OUT_OF_RANGE;
    }
    
    return ANALOG_SENSOR_OK;
}

float AnalogVoltageManager::voltageToScaledValue(int sensorIndex, float voltage) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    
    // Convert 0-10V to percentage based on sensor configuration
    float percentage = (voltage / 10.0) * 100.0;
    
    // Apply sensor-specific scaling
    float range = sensors[sensorIndex].maxValue - sensors[sensorIndex].minValue;
    float scaledValue = sensors[sensorIndex].minValue + (percentage / 100.0) * range;
    
    // Clamp to valid range
    return constrain(scaledValue, sensors[sensorIndex].minValue, sensors[sensorIndex].maxValue);
}

void AnalogVoltageManager::updateSensorStatus(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    if (readings[sensorIndex].status != ANALOG_SENSOR_OK) {
        errorCount[sensorIndex]++;
        
        if (DEBUG_ENABLED && errorCount[sensorIndex] % 10 == 1) {
            // // Serial.printf("[AV] Sensor %d (%s) error: %s\n", 
            //              sensorIndex, 
            //              sensors[sensorIndex].location.c_str(),
            //              getStatusString(sensorIndex).c_str());
        }
    }
}

// Data access methods
AnalogReading AnalogVoltageManager::getReading(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) {
        return {0, 0.0, 0.0, 0.0, ANALOG_SENSOR_ERROR, 0, false};
    }
    return readings[sensorIndex];
}

AnalogReading* AnalogVoltageManager::getAllReadings() {
    return readings;
}

float AnalogVoltageManager::getScaledValue(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    return readings[sensorIndex].scaledValue;
}

float AnalogVoltageManager::getVoltage(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    return readings[sensorIndex].calibratedVoltage;
}

float AnalogVoltageManager::getRawVoltage(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    return readings[sensorIndex].voltage;
}

String AnalogVoltageManager::getLocation(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "Unknown";
    return sensors[sensorIndex].location;
}

String AnalogVoltageManager::getUnit(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "";
    return sensors[sensorIndex].unit;
}

// Status and diagnostics
bool AnalogVoltageManager::isInitialized() {
    return initialized;
}

bool AnalogVoltageManager::isSensorEnabled(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    return sensors[sensorIndex].enabled;
}

AnalogSensorStatus AnalogVoltageManager::getSensorStatus(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return ANALOG_SENSOR_ERROR;
    return readings[sensorIndex].status;
}

String AnalogVoltageManager::getStatusString(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "INVALID";
    
    switch (readings[sensorIndex].status) {
        case ANALOG_SENSOR_OK: return "OK";
        case ANALOG_SENSOR_ERROR: return "ERROR";
        case ANALOG_SENSOR_DISCONNECTED: return "DISCONNECTED";
        case ANALOG_SENSOR_OUT_OF_RANGE: return "OUT_OF_RANGE";
        default: return "UNKNOWN";
    }
}

bool AnalogVoltageManager::hasErrors() {
    for (int i = 0; i < 3; i++) {
        if (sensors[i].enabled && readings[i].status != ANALOG_SENSOR_OK) {
            return true;
        }
    }
    return false;
}

unsigned long AnalogVoltageManager::getErrorCount(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0;
    return errorCount[sensorIndex];
}

unsigned long AnalogVoltageManager::getTotalReadings() {
    return totalReadings;
}

// Alarm checking
bool AnalogVoltageManager::isLowValue(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    if (!sensors[sensorIndex].enabled || readings[sensorIndex].status != ANALOG_SENSOR_OK) return false;
    
    return readings[sensorIndex].scaledValue < sensors[sensorIndex].lowThreshold;
}

bool AnalogVoltageManager::isHighValue(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    if (!sensors[sensorIndex].enabled || readings[sensorIndex].status != ANALOG_SENSOR_OK) return false;
    
    return readings[sensorIndex].scaledValue > sensors[sensorIndex].highThreshold;
}

String AnalogVoltageManager::getAlarmStatus() {
    String alarms = "";
    
    for (int i = 0; i < 3; i++) {
        if (!sensors[i].enabled) continue;
        
        if (isLowValue(i)) {
            if (alarms.length() > 0) alarms += ", ";
            alarms += sensors[i].location + " LOW";
        }
        
        if (isHighValue(i)) {
            if (alarms.length() > 0) alarms += ", ";
            alarms += sensors[i].location + " HIGH";
        }
    }
    
    return alarms.length() > 0 ? alarms : "NORMAL";
}

// Calibration and maintenance
void AnalogVoltageManager::calibrateSensor(int sensorIndex, float actualValue, float measuredVoltage) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    // This is a placeholder for more advanced calibration
    // For now, just log the calibration attempt
    if (DEBUG_ENABLED) {
        // // Serial.printf("[AV] Calibration point for sensor %d: %.1f%s = %.2fV\n", 
        //              sensorIndex, actualValue, sensors[sensorIndex].unit.c_str(), measuredVoltage);
    }
}

void AnalogVoltageManager::resetErrorCounts() {
    for (int i = 0; i < 3; i++) {
        errorCount[i] = 0;
    }
    totalReadings = 0;
    
    if (DEBUG_ENABLED) {
        // // Serial.println("[AV] Error counts reset");
    }
}

String AnalogVoltageManager::getInfo() {
    String info = "Analog Voltage Manager Status:\n";
    info += "Initialized: " + String(initialized ? "Yes" : "No") + "\n";
    info += "Total Readings: " + String(totalReadings) + "\n";
    info += "Read Interval: " + String(readInterval) + "ms\n\n";
    
    for (int i = 0; i < 3; i++) {
        info += "Sensor " + String(i) + " (" + sensors[i].location + "):\n";
        info += "  Enabled: " + String(sensors[i].enabled ? "Yes" : "No") + "\n";
        info += "  Value: " + String(readings[i].scaledValue, 1) + sensors[i].unit + "\n";
        info += "  Voltage: " + String(readings[i].calibratedVoltage, 2) + "V\n";
        info += "  Status: " + getStatusString(i) + "\n";
        info += "  Errors: " + String(errorCount[i]) + "\n\n";
    }
    
    return info;
}

void AnalogVoltageManager::logSensorData() {
    if (!initialized || !loggingEnabled || !sdMgr.isMounted()) return;
    
    // Create CSV format log entry
    String logEntry = "";
    
    // Add current readings for all sensors
    for (int i = 0; i < 3; i++) {
        if (i > 0) logEntry += ",";
        
        AnalogReading reading = readings[i];
        logEntry += String(reading.scaledValue, 2) + "," + 
                   String(reading.calibratedVoltage, 3) + "," +
                   String(reading.rawADC) + "," +
                   getStatusString(i);
    }
    
    // Add alarm status
    String alarmStatus = getAlarmStatus();
    if (alarmStatus == "NORMAL") alarmStatus = "OK";
    logEntry += "," + alarmStatus;
    
    // Log to SD card with timestamp
    if (sdMgr.logDataWithTimestamp("AV," + logEntry)) {
        if (DEBUG_ENABLED) {
            // // Serial.println("[AV] Data logged: " + logEntry);
        }
    } else {
        if (DEBUG_ENABLED) {
            // // Serial.println("[AV] Failed to log data to SD card");
        }
    }
}

// Advanced filtering configuration functions
void AnalogVoltageManager::setSmoothingFactor(int sensorIndex, float factor) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    // Clamp factor between 0.01 and 1.0
    factor = max(0.01f, min(1.0f, factor));
    sensors[sensorIndex].smoothingFactor = factor;
    
    if (DEBUG_ENABLED) {
        // // Serial.printf("[AV] Sensor %d smoothing factor set to %.2f\n", sensorIndex, factor);
    }
}

void AnalogVoltageManager::enableOutlierDetection(int sensorIndex, bool enable, float threshold) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    sensors[sensorIndex].outlierDetection = enable;
    if (enable) {
        // Clamp threshold between 5% and 50%
        sensors[sensorIndex].outlierThreshold = max(5.0f, min(50.0f, threshold));
    }
    
    if (DEBUG_ENABLED) {
        // // Serial.printf("[AV] Sensor %d outlier detection %s (threshold: %.1f%%)\n", 
        //              sensorIndex, enable ? "enabled" : "disabled", 
        //              sensors[sensorIndex].outlierThreshold);
    }
}

void AnalogVoltageManager::resetFilter(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    emaFilteredValue[sensorIndex] = 0.0;
    lastValidReading[sensorIndex] = 0.0;
    filterInitTime[sensorIndex] = 0;
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Filter reset for sensor %d\n", sensorIndex);
    }
}

// Advanced sensor health monitoring functions
void AnalogVoltageManager::updateSensorHealth(int sensorIndex, float voltage) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    SensorHealthData &health = healthData[sensorIndex];
    
    // Update min/max values
    if (health.minVoltage == 0.0 || voltage < health.minVoltage) {
        health.minVoltage = voltage;
    }
    if (voltage > health.maxVoltage) {
        health.maxVoltage = voltage;
    }
    
    // Update circular buffer of last values for stuck detection
    health.lastValues[health.valueIndex] = voltage;
    health.valueIndex = (health.valueIndex + 1) % 10;
    
    // Calculate simple variance from last 10 readings
    float sum = 0, sumSq = 0;
    for (int i = 0; i < 10; i++) {
        sum += health.lastValues[i];
        sumSq += health.lastValues[i] * health.lastValues[i];
    }
    float mean = sum / 10.0;
    health.variance = (sumSq / 10.0) - (mean * mean);
    
    // Check for significant change
    if (abs(voltage - lastValidReading[sensorIndex]) > 0.2) { // 0.2V significant change
        health.lastChangeTime = millis();
    }
}

void AnalogVoltageManager::evaluateSensorHealth(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    SensorHealthData &health = healthData[sensorIndex];
    
    // Reset flags
    health.isDead = false;
    health.isStuck = false;
    health.hasAnomalies = false;
    
    // Evaluate based on multiple criteria
    unsigned long timeSinceChange = millis() - health.lastChangeTime;
    
    // Dead sensor criteria
    if (health.minVoltage < 0.05 && health.maxVoltage < 0.05) {
        health.isDead = true;
    }
    
    // Stuck sensor criteria (no change for 5 minutes and low variance)
    if (timeSinceChange > 300000 && health.variance < 0.001) {
        health.isStuck = true;
    }
    
    // Anomaly detection (very high error count)
    if (errorCount[sensorIndex] > totalReadings * 0.1) { // 10% error rate
        health.hasAnomalies = true;
    }
    
    // Calculate overall health score
    health.healthScore = calculateHealthScore(sensorIndex);
    
    if (DEBUG_ENABLED && (health.isDead || health.isStuck || health.hasAnomalies)) {
        // Serial.printf("[AV] Sensor %d health issue: Dead=%s, Stuck=%s, Anomalies=%s, Score=%.1f%%\n",
        //              sensorIndex, 
        //              health.isDead ? "YES" : "NO",
        //              health.isStuck ? "YES" : "NO", 
        //              health.hasAnomalies ? "YES" : "NO",
        //              health.healthScore);
    }
}

bool AnalogVoltageManager::detectStuckSensor(int sensorIndex, float voltage) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    
    SensorHealthData &health = healthData[sensorIndex];
    
    // Check if all last 10 readings are nearly identical
    bool allSame = true;
    for (int i = 0; i < 9; i++) {
        if (abs(health.lastValues[i] - health.lastValues[i+1]) > 0.01) {
            allSame = false;
            break;
        }
    }
    
    return allSame && (millis() - health.lastChangeTime > 180000); // 3 minutes
}

bool AnalogVoltageManager::detectDeadSensor(int sensorIndex, float voltage) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    
    // Multiple criteria for dead sensor detection
    bool veryLowVoltage = voltage < 0.05; // Below 50mV
    bool noVariance = healthData[sensorIndex].variance < 0.0001;
    bool longTimeNoChange = (millis() - healthData[sensorIndex].lastChangeTime) > 600000; // 10 minutes
    
    return veryLowVoltage && (noVariance || longTimeNoChange);
}

float AnalogVoltageManager::calculateHealthScore(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    
    SensorHealthData &health = healthData[sensorIndex];
    float score = 100.0;
    
    // Deduct points for various issues
    if (health.isDead) score -= 100.0;
    else if (health.isStuck) score -= 30.0;
    
    if (health.hasAnomalies) score -= 20.0;
    
    // Deduct based on error rate
    if (totalReadings > 0) {
        float errorRate = (float)errorCount[sensorIndex] / totalReadings;
        score -= errorRate * 50.0; // Up to 50 points for errors
    }
    
    // Deduct for low variance (indicates poor sensor response)
    if (health.variance < 0.01) score -= 10.0;
    
    return max(0.0f, min(100.0f, score));
}

// Enhanced calibration functions
void AnalogVoltageManager::setCalibration(int sensorIndex, float offset, float gain) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    sensors[sensorIndex].offsetCorrection = offset;
    sensors[sensorIndex].gainCorrection = gain;
    sensors[sensorIndex].calibrated = true;
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Sensor %d calibration set: offset=%.3f, gain=%.3f\n", 
        //              sensorIndex, offset, gain);
    }
}

void AnalogVoltageManager::resetCalibration(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    sensors[sensorIndex].offsetCorrection = 0.0;
    sensors[sensorIndex].gainCorrection = 1.0;
    sensors[sensorIndex].calibrated = false;
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Sensor %d calibration reset\n", sensorIndex);
    }
}

bool AnalogVoltageManager::isCalibrated(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    return sensors[sensorIndex].calibrated;
}

// Sensor health reporting
float AnalogVoltageManager::getSensorHealth(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    return healthData[sensorIndex].healthScore;
}

bool AnalogVoltageManager::isSensorStuck(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    return healthData[sensorIndex].isStuck;
}

bool AnalogVoltageManager::isSensorDead(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    return healthData[sensorIndex].isDead;
}

String AnalogVoltageManager::getHealthReport() {
    String report = "=== Sensor Health Report ===\n";
    
    for (int i = 0; i < 3; i++) {
        if (!sensors[i].enabled) continue;
        
        SensorHealthData &health = healthData[i];
        report += "Sensor " + String(i) + " (" + sensors[i].location + "):\n";
        report += "  Health Score: " + String(health.healthScore, 1) + "%\n";
        report += "  Status: ";
        
        if (health.isDead) report += "DEAD ";
        if (health.isStuck) report += "STUCK ";
        if (health.hasAnomalies) report += "ANOMALIES ";
        if (!health.isDead && !health.isStuck && !health.hasAnomalies) report += "HEALTHY";
        
        report += "\n";
        report += "  Voltage Range: " + String(health.minVoltage, 2) + "V - " + String(health.maxVoltage, 2) + "V\n";
        report += "  Variance: " + String(health.variance, 4) + "\n";
        report += "  Errors: " + String(errorCount[i]) + "/" + String(totalReadings) + "\n";
        report += "  Calibrated: " + String(sensors[i].calibrated ? "YES" : "NO") + "\n\n";
    }
    
    return report;
}

void AnalogVoltageManager::resetHealthData(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    healthData[sensorIndex] = {0.0, 0.0, 0.0, {0}, 0, 0, 0, millis(), false, false, false, 100.0};
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Health data reset for sensor %d\n", sensorIndex);
    }
}

// Enhanced sensor configuration functions
void AnalogVoltageManager::configureSensorIdentity(int sensorIndex, const String& sensorId, 
                                                   const String& manufacturer, const String& model,
                                                   const String& serialNumber, const String& installationDate,
                                                   const String& description, const String& group,
                                                   const String& tags) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    sensors[sensorIndex].sensorId = sensorId;
    sensors[sensorIndex].manufacturer = manufacturer;
    sensors[sensorIndex].model = model;
    sensors[sensorIndex].serialNumber = serialNumber;
    sensors[sensorIndex].installationDate = installationDate;
    sensors[sensorIndex].description = description;
    sensors[sensorIndex].group = group;
    sensors[sensorIndex].tags = tags;
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Sensor %d identity updated: ID=%s, Mfg=%s, Model=%s\n", 
        //              sensorIndex, sensorId.c_str(), manufacturer.c_str(), model.c_str());
    }
}

void AnalogVoltageManager::setAlarmConfig(int sensorIndex, int severity, const String& customMessage, bool enabled) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    sensors[sensorIndex].alarmSeverity = max(1, min(4, severity)); // Clamp to 1-4
    sensors[sensorIndex].customAlarmMessage = customMessage;
    sensors[sensorIndex].alarmEnabled = enabled;
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Sensor %d alarm config: Severity=%d, Enabled=%s\n", 
        //              sensorIndex, severity, enabled ? "YES" : "NO");
    }
}

// Enhanced data access functions
String AnalogVoltageManager::getSensorId(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "";
    return sensors[sensorIndex].sensorId;
}

String AnalogVoltageManager::getManufacturer(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "";
    return sensors[sensorIndex].manufacturer;
}

String AnalogVoltageManager::getModel(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "";
    return sensors[sensorIndex].model;
}

String AnalogVoltageManager::getSerialNumber(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "";
    return sensors[sensorIndex].serialNumber;
}

String AnalogVoltageManager::getInstallationDate(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "";
    return sensors[sensorIndex].installationDate;
}

String AnalogVoltageManager::getDescription(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "";
    return sensors[sensorIndex].description;
}

String AnalogVoltageManager::getGroup(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "";
    return sensors[sensorIndex].group;
}

String AnalogVoltageManager::getTags(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "";
    return sensors[sensorIndex].tags;
}

String AnalogVoltageManager::getSensorInfo(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "{}";
    
    String info = "{";
    info += "\"sensor_id\":\"" + sensors[sensorIndex].sensorId + "\",";
    info += "\"location\":\"" + sensors[sensorIndex].location + "\",";
    info += "\"manufacturer\":\"" + sensors[sensorIndex].manufacturer + "\",";
    info += "\"model\":\"" + sensors[sensorIndex].model + "\",";
    info += "\"serial_number\":\"" + sensors[sensorIndex].serialNumber + "\",";
    info += "\"installation_date\":\"" + sensors[sensorIndex].installationDate + "\",";
    info += "\"description\":\"" + sensors[sensorIndex].description + "\",";
    info += "\"group\":\"" + sensors[sensorIndex].group + "\",";
    info += "\"tags\":\"" + sensors[sensorIndex].tags + "\",";
    info += "\"unit\":\"" + sensors[sensorIndex].unit + "\",";
    info += "\"range\":\"" + String(sensors[sensorIndex].minValue) + "-" + String(sensors[sensorIndex].maxValue) + "\",";
    info += "\"calibrated\":" + String(sensors[sensorIndex].calibrated ? "true" : "false") + ",";
    info += "\"last_calibration\":\"" + sensors[sensorIndex].lastCalibrationDate + "\",";
    info += "\"calibrated_by\":\"" + sensors[sensorIndex].calibratedBy + "\",";
    info += "\"alarm_severity\":" + String(sensors[sensorIndex].alarmSeverity) + ",";
    info += "\"alarm_enabled\":" + String(sensors[sensorIndex].alarmEnabled ? "true" : "false");
    info += "}";
    
    return info;
}

// Real-time streaming and notification functions
void AnalogVoltageManager::enableStreaming(bool enable) {
    streamingEnabled = enable;
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Real-time streaming %s\n", enable ? "enabled" : "disabled");
    }
}

void AnalogVoltageManager::setStreamInterval(unsigned long interval) {
    streamInterval = max(interval, 100UL);  // Minimum 100ms
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Stream interval set to %lu ms\n", streamInterval);
    }
}

void AnalogVoltageManager::generateDataStreamEvent() {
    if (!initialized || !streamingEnabled) return;
    
    // Generate JSON stream data for WebSocket
    String streamData = "{";
    streamData += "\"timestamp\":" + String(millis()) + ",";
    streamData += "\"type\":\"sensor_data\",";
    streamData += "\"sensors\":[";
    
    for (int i = 0; i < 3; i++) {
        if (i > 0) streamData += ",";
        
        streamData += "{";
        streamData += "\"id\":" + String(i) + ",";
        streamData += "\"sensor_id\":\"" + sensors[i].sensorId + "\",";
        streamData += "\"location\":\"" + sensors[i].location + "\",";
        streamData += "\"value\":" + String(readings[i].scaledValue, 2) + ",";
        streamData += "\"unit\":\"" + sensors[i].unit + "\",";
        streamData += "\"voltage\":" + String(readings[i].calibratedVoltage, 3) + ",";
        streamData += "\"status\":\"" + getStatusString(i) + "\",";
        streamData += "\"health_score\":" + String(healthData[i].healthScore, 1) + ",";
        streamData += "\"is_alarm\":" + String((isLowValue(i) || isHighValue(i)) ? "true" : "false");
        streamData += "}";
    }
    
    streamData += "]}";
    
    // Notify WebSocket clients (this will be implemented in web_server.cpp)
    notifyWebSocketClients();
    
    if (DEBUG_ENABLED && totalReadings % 60 == 0) { // Log every 60 readings
        // Serial.println("[AV] Stream data generated: " + streamData.substring(0, 100) + "...");
    }
}

void AnalogVoltageManager::triggerAlarmNotification(int sensorIndex, const String& alarmType) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    static unsigned long lastAlarmTime[3] = {0, 0, 0};
    unsigned long currentTime = millis();
    
    // Debounce alarms - minimum 30 seconds between same alarm type
    if (currentTime - lastAlarmTime[sensorIndex] < 30000) return;
    
    lastAlarmTime[sensorIndex] = currentTime;
    
    String alarmData = "{";
    alarmData += "\"timestamp\":" + String(currentTime) + ",";
    alarmData += "\"type\":\"alarm\",";
    alarmData += "\"sensor_id\":" + String(sensorIndex) + ",";
    alarmData += "\"sensor_name\":\"" + sensors[sensorIndex].location + "\",";
    alarmData += "\"alarm_type\":\"" + alarmType + "\",";
    alarmData += "\"severity\":" + String(sensors[sensorIndex].alarmSeverity) + ",";
    alarmData += "\"current_value\":" + String(readings[sensorIndex].scaledValue, 2) + ",";
    alarmData += "\"unit\":\"" + sensors[sensorIndex].unit + "\",";
    
    String message = sensors[sensorIndex].customAlarmMessage;
    if (message == "") {
        message = alarmType + " on " + sensors[sensorIndex].location;
    }
    alarmData += "\"message\":\"" + message + "\"";
    alarmData += "}";
    
    // Log alarm to SD card
    if (sdMgr.isMounted()) {
        sdMgr.logDataWithTimestamp("ALARM," + alarmData);
    }
    
    // Send webhook notification
    String sensorId = "sensor_" + String(sensorIndex);
    if (alarmType.indexOf("HIGH") >= 0) {
        webhookHandler.sendAlarmTriggered(sensorId, alarmType, 
                                        readings[sensorIndex].scaledValue, 
                                        sensors[sensorIndex].highThreshold);
    } else if (alarmType.indexOf("LOW") >= 0) {
        webhookHandler.sendAlarmTriggered(sensorId, alarmType, 
                                        readings[sensorIndex].scaledValue, 
                                        sensors[sensorIndex].lowThreshold);
    } else {
        // Health-based alarms (SENSOR_DEAD, SENSOR_STUCK, etc.)
        webhookHandler.sendHealthAlert(sensorId, message, "critical");
    }
    
    // Notify WebSocket clients
    notifyWebSocketClients();
    
    if (DEBUG_ENABLED) {
        // Serial.println("[AV] ALARM: " + alarmType + " on sensor " + String(sensorIndex) + " (" + sensors[sensorIndex].location + ")");
        // Serial.println("[AV] Alarm data: " + alarmData);
    }
}

void AnalogVoltageManager::notifyWebSocketClients() {
    // Call the web server's WebSocket broadcast function
    extern WebServerHandler webServer;
    
    // Generate real-time data for WebSocket clients
    String streamData = "{";
    streamData += "\"timestamp\":" + String(millis()) + ",";
    streamData += "\"type\":\"sensor_stream\",";
    streamData += "\"sensors\":[";
    
    for (int i = 0; i < 3; i++) {
        if (i > 0) streamData += ",";
        
        streamData += "{";
        streamData += "\"id\":" + String(i) + ",";
        streamData += "\"location\":\"" + sensors[i].location + "\",";
        streamData += "\"value\":" + String(readings[i].scaledValue, 2) + ",";
        streamData += "\"unit\":\"" + sensors[i].unit + "\",";
        streamData += "\"voltage\":" + String(readings[i].calibratedVoltage, 3) + ",";
        streamData += "\"status\":\"" + getStatusString(i) + "\",";
        streamData += "\"health_score\":" + String(healthData[i].healthScore, 1) + ",";
        streamData += "\"is_alarm\":" + String((isLowValue(i) || isHighValue(i)) ? "true" : "false");
        streamData += "}";
    }
    
    streamData += "]}";
    
    // Broadcast to WebSocket clients
    webServer.broadcastToWebSocket(streamData);
    
    if (DEBUG_ENABLED && totalReadings % 100 == 0) {
        // Serial.printf("[AV] WebSocket notification sent to %d clients\n", 
        //              webServer.getWebSocketClientCount());
    }
}

// Additional health and alarm getters for WebSocket
float AnalogVoltageManager::getHealthScore(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    return healthData[sensorIndex].healthScore;
}

bool AnalogVoltageManager::isDeadSensor(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    return healthData[sensorIndex].isDead;
}

bool AnalogVoltageManager::isStuckSensor(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    return healthData[sensorIndex].isStuck;
}

float AnalogVoltageManager::getLowThreshold(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    return sensors[sensorIndex].lowThreshold;
}

float AnalogVoltageManager::getHighThreshold(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    return sensors[sensorIndex].highThreshold;
}

// Sensor simulation functions
void AnalogVoltageManager::enableSimulation(bool enable) {
    simulationEnabled = enable;
    
    if (enable) {
        // Initialize simulation start times
        unsigned long currentTime = millis();
        for (int i = 0; i < 3; i++) {
            simulationStartTime[i] = currentTime;
        }
    }
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Sensor simulation %s\n", enable ? "enabled" : "disabled");
    }
}

void AnalogVoltageManager::setSimulationMode(int sensorIndex, const String& mode) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    simulationMode[sensorIndex] = mode;
    sensorSimulated[sensorIndex] = true;
    simulationStartTime[sensorIndex] = millis();
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Sensor %d simulation mode set to: %s\n", sensorIndex, mode.c_str());
    }
}

void AnalogVoltageManager::setSimulationValue(int sensorIndex, float value) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    simulationValue[sensorIndex] = constrain(value, sensors[sensorIndex].minValue, sensors[sensorIndex].maxValue);
    sensorSimulated[sensorIndex] = true;
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Sensor %d simulation value set to: %.2f\n", sensorIndex, simulationValue[sensorIndex]);
    }
}

void AnalogVoltageManager::setSimulationPattern(int sensorIndex, const String& pattern, float amplitude, float frequency) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    simulationMode[sensorIndex] = pattern;
    simulationAmplitude[sensorIndex] = amplitude;
    simulationFrequency[sensorIndex] = frequency;
    sensorSimulated[sensorIndex] = true;
    simulationStartTime[sensorIndex] = millis();
    
    if (DEBUG_ENABLED) {
        // Serial.printf("[AV] Sensor %d simulation pattern: %s, amplitude: %.2f, frequency: %.3f Hz\n", 
        //              sensorIndex, pattern.c_str(), amplitude, frequency);
    }
}

bool AnalogVoltageManager::isSimulationEnabled() {
    return simulationEnabled;
}

bool AnalogVoltageManager::isSensorSimulated(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    return simulationEnabled && sensorSimulated[sensorIndex];
}

float AnalogVoltageManager::generateSimulatedReading(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    
    float baseValue = simulationValue[sensorIndex];
    
    if (simulationMode[sensorIndex] == "fixed") {
        return baseValue;
    }
    
    return applySimulationPattern(sensorIndex, baseValue);
}

float AnalogVoltageManager::applySimulationPattern(int sensorIndex, float baseValue) {
    if (sensorIndex < 0 || sensorIndex >= 3) return baseValue;
    
    unsigned long currentTime = millis();
    float elapsedSeconds = (currentTime - simulationStartTime[sensorIndex]) / 1000.0;
    float result = baseValue;
    
    String mode = simulationMode[sensorIndex];
    float amplitude = simulationAmplitude[sensorIndex];
    float frequency = simulationFrequency[sensorIndex];
    
    if (mode == "sine") {
        result = baseValue + amplitude * sin(2.0 * PI * frequency * elapsedSeconds);
        
    } else if (mode == "triangle") {
        float period = 1.0 / frequency;
        float phase = fmod(elapsedSeconds, period) / period;
        if (phase < 0.5) {
            result = baseValue + amplitude * (4.0 * phase - 1.0);
        } else {
            result = baseValue + amplitude * (3.0 - 4.0 * phase);
        }
        
    } else if (mode == "ramp") {
        float period = 1.0 / frequency;
        float phase = fmod(elapsedSeconds, period) / period;
        result = baseValue + amplitude * (2.0 * phase - 1.0);
        
    } else if (mode == "noise") {
        // Pseudo-random noise using millis()
        result = baseValue + amplitude * ((random(0, 2000) - 1000) / 1000.0);
        
    } else if (mode == "step") {
        // Square wave
        float period = 1.0 / frequency;
        float phase = fmod(elapsedSeconds, period) / period;
        result = baseValue + amplitude * (phase < 0.5 ? -1.0 : 1.0);
    }
    
    // Constrain to sensor range
    result = constrain(result, sensors[sensorIndex].minValue, sensors[sensorIndex].maxValue);
    
    return result;
}