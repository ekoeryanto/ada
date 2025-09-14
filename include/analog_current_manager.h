#ifndef ANALOG_CURRENT_MANAGER_H
#define ANALOG_CURRENT_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "pins_config.h"

enum CurrentSensorStatus {
    CURRENT_SENSOR_OK,
    CURRENT_SENSOR_ERROR,
    CURRENT_SENSOR_DISCONNECTED,
    CURRENT_SENSOR_OUT_OF_RANGE,
    CURRENT_SENSOR_LOOP_BROKEN,
    CURRENT_SENSOR_OVERCURRENT
};

struct CurrentReading {
    int rawADC;
    float voltage;              // Raw ESP32 voltage (0-3.3V)
    float current;              // Calculated current in mA (4-20mA)
    float scaledValue;          // Scaled value based on sensor configuration (0-100% or custom range)
    CurrentSensorStatus status;
    unsigned long timestamp;
    bool valid;
    float loopResistance;       // Calculated loop resistance for diagnostics
    float signalQuality;        // Signal quality score (0-100%)
};

struct CurrentSensorConfig {
    // Basic identification
    String location;            // Sensor location/name
    String unit;                // Unit of measurement (%, bar, psi, level, flow, etc.)
    
    // Extended sensor identity
    String sensorId;            // Unique identifier (e.g., "AI1_LEVEL_001")
    String manufacturer;        // Manufacturer name (e.g., "Rosemount", "Endress+Hauser")
    String model;               // Model number (e.g., "3051CD", "FMU30")
    String serialNumber;        // Serial number
    String installationDate;    // Installation date (ISO format)
    String description;         // Detailed description
    String group;               // Sensor group (e.g., "Tank_A", "Line_1")
    String tags;                // Tags for filtering (e.g., "critical,level,tank")
    
    // Current loop configuration
    float currentMin;           // Minimum current (typically 4.0mA)
    float currentMax;           // Maximum current (typically 20.0mA)
    float senseResistor;        // Sense resistor value in ohms (typically 120-250Ω)
    float amplifierGain;        // Current-to-voltage amplifier gain (typically 1.0-2.0)
    
    // Measurement configuration  
    float minValue;             // Minimum scale value (4mA corresponds to)
    float maxValue;             // Maximum scale value (20mA corresponds to)
    float lowThreshold;         // Low value alarm threshold
    float highThreshold;        // High value alarm threshold
    bool enabled;
    
    // Advanced filtering options
    float smoothingFactor;      // EMA smoothing factor (0.1-1.0)
    bool outlierDetection;      // Enable outlier detection and rejection
    float outlierThreshold;     // Outlier detection threshold (% deviation)
    
    // Calibration parameters
    float offsetCorrection;     // Zero offset correction (mA)
    float gainCorrection;       // Gain correction factor
    bool calibrated;            // Is sensor calibrated?
    String lastCalibrationDate; // Last calibration date
    String calibratedBy;        // Who performed calibration
    
    // Loop diagnostics configuration
    bool loopDiagnostics;       // Enable loop resistance monitoring
    float expectedLoopResistance; // Expected total loop resistance (Ω)
    float loopTolerancePercent;   // Tolerance for loop resistance (%)
    
    // Alarm configuration
    int alarmSeverity;          // 1=Info, 2=Warning, 3=Critical, 4=Emergency
    String customAlarmMessage;  // Custom alarm message
    bool alarmEnabled;          // Enable/disable alarms for this sensor
    bool loopAlarmEnabled;      // Enable loop diagnostics alarms
};

struct CurrentSensorHealthData {
    float minCurrent;           // Minimum current seen
    float maxCurrent;           // Maximum current seen
    float variance;             // Running variance
    float lastValues[10];       // Last 10 readings for stuck detection
    int valueIndex;             // Current index in lastValues array
    unsigned long stuckCount;   // Count of consecutive identical readings
    unsigned long anomalyCount; // Count of anomalies detected
    unsigned long lastChangeTime; // Last time value changed significantly
    unsigned long loopErrorCount; // Count of loop diagnostics errors
    bool isStuck;               // Sensor appears stuck
    bool isOpenLoop;            // Loop appears open/disconnected
    bool hasAnomalies;          // Recent anomalies detected
    bool loopDegraded;          // Loop resistance outside tolerance
    float healthScore;          // Overall health score (0-100%)
    float avgLoopResistance;    // Average loop resistance
    float lastLoopResistance;   // Last calculated loop resistance
};

class AnalogCurrentManager {
private:
    // Sensor configurations for 3 current loop inputs
    CurrentSensorConfig sensors[3];
    
    // Current readings
    CurrentReading readings[3];
    
    // Advanced filtering
    float emaFilteredValue[3];      // EMA filtered values
    float lastValidReading[3];      // Last valid reading for outlier detection
    unsigned long filterInitTime[3]; // Time when filter was initialized
    static const int FILTER_SAMPLES = 10; // Samples needed for filter initialization
    
    // Sensor health monitoring
    CurrentSensorHealthData healthData[3]; // Health tracking for each sensor
    unsigned long lastHealthCheck;  // Last time health was evaluated
    static const unsigned long HEALTH_CHECK_INTERVAL = 30000; // 30 seconds
    
    // Timing and sampling
    unsigned long lastReadTime;
    unsigned long readInterval;
    unsigned long lastLogTime;
    unsigned long logInterval;
    static const int NUM_SAMPLES = 20;  // Number of samples for smoothing
    
    // Status tracking
    bool initialized;
    unsigned long errorCount[3];
    unsigned long totalReadings;
    bool loggingEnabled;
    
    // Real-time streaming
    unsigned long lastStreamTime;
    unsigned long streamInterval;
    bool streamingEnabled;
    static const unsigned long DEFAULT_STREAM_INTERVAL = 1000; // 1 second
    
    // Sensor simulation
    bool simulationEnabled;
    bool sensorSimulated[3];
    String simulationMode[3];      // "fixed", "sine", "ramp", "noise", "triangle"
    float simulationValue[3];      // Fixed value or base value for patterns
    float simulationAmplitude[3];  // Amplitude for wave patterns
    float simulationFrequency[3];  // Frequency for wave patterns
    unsigned long simulationStartTime[3]; // Start time for pattern calculations
    
    // Private methods
    float convertVoltageToCurrent(int sensorIndex, float voltage);
    float convertCurrentToScaled(int sensorIndex, float current);
    int readAnalogSmoothed(int pin);
    float applyEMAFilter(int sensorIndex, float newValue);
    bool isOutlier(int sensorIndex, float value);
    float processCurrentReading(int sensorIndex, int rawADC);
    CurrentSensorStatus validateReading(int sensorIndex, float current);
    float calculateLoopResistance(int sensorIndex, float current, float voltage);
    float calculateSignalQuality(int sensorIndex, float current);
    void updateSensorStatus(int sensorIndex);
    void logSensorData();
    
    // Advanced sensor monitoring
    void updateSensorHealth(int sensorIndex, float current);
    void evaluateSensorHealth(int sensorIndex);
    bool detectStuckSensor(int sensorIndex, float current);
    bool detectOpenLoop(int sensorIndex, float current);
    bool detectLoopDegradation(int sensorIndex, float loopResistance);
    float calculateHealthScore(int sensorIndex);
    
    // Real-time streaming and notifications
    void notifyWebSocketClients();
    void triggerAlarmNotification(int sensorIndex, const String& alarmType);
    void generateDataStreamEvent();
    
    // Simulation methods
    float generateSimulatedReading(int sensorIndex);
    float applySimulationPattern(int sensorIndex, float baseValue);

public:
    AnalogCurrentManager();
    
    // Initialization and configuration
    bool begin();
    void configureSensor(int sensorIndex, const String& location, const String& unit,
                        float minValue, float maxValue, float senseResistor = 120.0, float ampGain = 2.0,
                        float lowThreshold = 20.0, float highThreshold = 80.0);
    void configureSensorIdentity(int sensorIndex, const String& sensorId, 
                               const String& manufacturer, const String& model,
                               const String& serialNumber, const String& installationDate = "",
                               const String& description = "", const String& group = "",
                               const String& tags = "");
    void configureCurrentLoop(int sensorIndex, float currentMin = 4.0, float currentMax = 20.0,
                             float senseResistor = 120.0, float ampGain = 2.0);
    void configureLoopDiagnostics(int sensorIndex, bool enable = true, float expectedResistance = 250.0, 
                                 float tolerancePercent = 15.0);
    void setAlarmConfig(int sensorIndex, int severity, const String& customMessage = "", bool enabled = true);
    void setReadInterval(unsigned long interval);
    void setLogInterval(unsigned long interval);
    void enableSensor(int sensorIndex, bool enable = true);
    void enableLogging(bool enable = true);
    
    // Real-time streaming configuration
    void enableStreaming(bool enable = true);
    void setStreamInterval(unsigned long interval);
    
    // Sensor simulation functions
    void enableSimulation(bool enable);
    void setSimulationMode(int sensorIndex, const String& mode);
    void setSimulationValue(int sensorIndex, float value);
    void setSimulationPattern(int sensorIndex, const String& pattern, float amplitude, float frequency);
    bool isSimulationEnabled();
    bool isSensorSimulated(int sensorIndex);
    
    // Advanced filtering configuration
    void setSmoothingFactor(int sensorIndex, float factor);
    void enableOutlierDetection(int sensorIndex, bool enable = true, float threshold = 20.0);
    void resetFilter(int sensorIndex);
    
    // Main operations (non-blocking)
    void handle();
    bool isReadingReady();
    void requestReading();
    
    // Data access
    CurrentReading getReading(int sensorIndex);
    CurrentReading* getAllReadings();
    float getScaledValue(int sensorIndex);
    float getCurrent(int sensorIndex);
    float getVoltage(int sensorIndex);
    float getCurrentPercent(int sensorIndex);  // Return 4-20mA as 0-100%
    String getLocation(int sensorIndex);
    String getUnit(int sensorIndex);
    
    // Enhanced sensor information
    String getSensorId(int sensorIndex);
    String getManufacturer(int sensorIndex);
    String getModel(int sensorIndex);
    String getSerialNumber(int sensorIndex);
    String getInstallationDate(int sensorIndex);
    String getDescription(int sensorIndex);
    String getGroup(int sensorIndex);
    String getTags(int sensorIndex);
    String getSensorInfo(int sensorIndex);  // Complete sensor info JSON
    
    // Current loop specific information
    float getLoopResistance(int sensorIndex);
    float getSignalQuality(int sensorIndex);
    bool isLoopHealthy(int sensorIndex);
    String getLoopDiagnostics(int sensorIndex);
    
    // Status and diagnostics
    bool isInitialized();
    bool isSensorEnabled(int sensorIndex);
    CurrentSensorStatus getSensorStatus(int sensorIndex);
    String getStatusString(int sensorIndex);
    bool hasErrors();
    unsigned long getErrorCount(int sensorIndex);
    unsigned long getTotalReadings();
    
    // Alarm checking
    bool isLowValue(int sensorIndex);
    bool isHighValue(int sensorIndex);
    bool isLoopError(int sensorIndex);
    bool isOpenLoop(int sensorIndex);
    String getAlarmStatus();
    
    // Calibration and maintenance
    void calibrateSensor(int sensorIndex, float actualValue, float measuredCurrent);
    void calibrateCurrentLoop(int sensorIndex, float known4mA, float known20mA);
    void setCalibration(int sensorIndex, float offset, float gain);
    void resetCalibration(int sensorIndex);
    bool isCalibrated(int sensorIndex);
    void resetErrorCounts();
    String getInfo();
    
    // Sensor health and diagnostics
    float getSensorHealth(int sensorIndex);
    bool isSensorStuck(int sensorIndex);
    bool isSensorOpenLoop(int sensorIndex);
    bool isLoopDegraded(int sensorIndex);
    String getHealthReport();
    void resetHealthData(int sensorIndex);
    
    // Additional health and alarm getters for WebSocket
    float getHealthScore(int sensorIndex);
    bool isDeadSensor(int sensorIndex);
    bool isStuckSensor(int sensorIndex);
    float getLowThreshold(int sensorIndex);
    float getHighThreshold(int sensorIndex);
    
    // Current loop specific methods
    bool validateCurrentRange(float current);
    float getCurrentAsPercent(float current);
    float percentToCurrent(float percent);
    String getCurrentLoopStatus(int sensorIndex);
};

// Global instance
extern AnalogCurrentManager analogCurrentMgr;

#endif // ANALOG_CURRENT_MANAGER_H