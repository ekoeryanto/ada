#pragma once

#include <Arduino.h>
#include <vector>
#include <map>
#include "analog_voltage_manager.h"
#include "analog_current_manager.h"
#include "digital_io_manager.h"
#include "modbus_manager.h"
#include "multi_rtu_modbus_manager.h"

// Unified sensor types
enum UnifiedSensorType {
    SENSOR_DIGITAL_INPUT,       // Digital input sensor
    SENSOR_DIGITAL_OUTPUT,      // Digital output (actuator)
    SENSOR_ANALOG_VOLTAGE,      // Analog voltage sensor (0-10V)
    SENSOR_ANALOG_CURRENT,      // Analog current sensor (4-20mA)
    SENSOR_MODBUS_RTU,          // Modbus RTU sensor
    SENSOR_VIRTUAL,             // Virtual/calculated sensor
    SENSOR_SYSTEM               // System status sensor
};

// Unified sensor status
enum UnifiedSensorStatus {
    SENSOR_STATUS_UNKNOWN,      // Unknown status
    SENSOR_STATUS_OK,           // Sensor working properly
    SENSOR_STATUS_WARNING,      // Sensor has warnings
    SENSOR_STATUS_ERROR,        // Sensor has errors
    SENSOR_STATUS_OFFLINE,      // Sensor is offline/disconnected
    SENSOR_STATUS_MAINTENANCE,  // Sensor in maintenance mode
    SENSOR_STATUS_CALIBRATING   // Sensor is being calibrated
};

// Unified sensor reading
struct UnifiedSensorReading {
    String sensorId;               // Unique sensor identifier
    String name;                   // Sensor name
    String description;            // Sensor description
    UnifiedSensorType type;        // Sensor type
    
    // Value data
    float value;                   // Primary value
    String unit;                   // Unit of measurement
    String stringValue;            // String representation
    
    // Additional values for complex sensors
    std::map<String, float> additionalValues; // e.g., {"temperature": 25.5, "humidity": 60.2}
    
    // Quality and status
    UnifiedSensorStatus status;    // Sensor status
    uint8_t quality;               // Data quality (0-100%)
    bool valid;                    // Data validity
    
    // Timestamps
    unsigned long timestamp;       // Reading timestamp
    unsigned long responseTime;    // Response time (ms)
    
    // Source information
    String sourceType;             // Source type (e.g., "ADS1115", "GPIO", "ModbusRTU")
    String sourceAddress;          // Source address (e.g., "I2C:0x48", "GPIO:4", "RTU0:1:1000")
    
    // Error information
    uint8_t errorCode;             // Error code (0 = no error)
    String errorMessage;           // Error message
    
    // Metadata
    String group;                  // Sensor group
    String location;               // Physical location
    std::map<String, String> tags; // Custom tags
};

// Unified sensor configuration
struct UnifiedSensorConfig {
    String sensorId;               // Unique sensor identifier
    String name;                   // Sensor name
    String description;            // Sensor description
    UnifiedSensorType type;        // Sensor type
    
    // Source configuration
    String sourceType;             // Source type
    String sourceAddress;          // Source address
    uint8_t sourceChannel;         // Source channel/pin
    
    // Modbus specific (if applicable)
    uint8_t modbusInterface;       // Modbus RTU interface ID
    uint8_t modbusSlaveId;         // Modbus slave ID
    String modbusRegister;         // Modbus register name
    
    // Scaling and calibration
    float scaleFactor;             // Scaling factor
    float offset;                  // Offset value
    float minValue;                // Minimum allowed value
    float maxValue;                // Maximum allowed value
    String unit;                   // Unit of measurement
    
    // Update configuration
    bool enabled;                  // Sensor enabled flag
    unsigned long updateInterval;  // Update interval (ms)
    bool autoUpdate;               // Auto-update flag
    
    // Alarm configuration
    bool alarmEnabled;             // Alarm enabled
    float alarmLowThreshold;       // Low alarm threshold
    float alarmHighThreshold;      // High alarm threshold
    String alarmMessage;           // Alarm message template
    
    // Logging configuration
    bool loggingEnabled;           // Logging enabled
    bool streamingEnabled;         // Streaming enabled
    
    // Metadata
    String group;                  // Sensor group
    String location;               // Physical location
    uint8_t priority;              // Priority (0-255, 0 = highest)
    std::map<String, String> tags; // Custom tags
    
    // Default constructor
    UnifiedSensorConfig() :
        sensorId(""), name(""), description(""), type(SENSOR_ANALOG_VOLTAGE),
        sourceType(""), sourceAddress(""), sourceChannel(0),
        modbusInterface(0), modbusSlaveId(1), modbusRegister(""),
        scaleFactor(1.0), offset(0.0), minValue(0.0), maxValue(100.0), unit(""),
        enabled(true), updateInterval(1000), autoUpdate(true),
        alarmEnabled(false), alarmLowThreshold(0.0), alarmHighThreshold(100.0), alarmMessage(""),
        loggingEnabled(true), streamingEnabled(false),
        group("default"), location(""), priority(128) {}
};

// Unified alarm event
struct UnifiedAlarmEvent {
    String sensorId;               // Sensor ID that triggered alarm
    String alarmType;              // Alarm type (HIGH, LOW, OFFLINE, etc.)
    String severity;               // Severity (INFO, WARNING, ERROR, CRITICAL)
    float currentValue;            // Current sensor value
    float thresholdValue;          // Threshold that was crossed
    String message;                // Alarm message
    unsigned long timestamp;       // Alarm timestamp
    bool acknowledged;             // Alarm acknowledgment status
    String acknowledgedBy;         // Who acknowledged the alarm
    unsigned long acknowledgedTime; // When was it acknowledged
};

// Unified Sensor Manager
class UnifiedSensorManager {
private:
    // Sensor configurations and readings
    std::vector<UnifiedSensorConfig> sensorConfigs;
    std::map<String, size_t> sensorIndexMap;  // sensorId -> config index
    std::map<String, UnifiedSensorReading> lastReadings; // sensorId -> last reading
    
    // Manager instances
    AnalogVoltageManager* analogVoltageMgr;
    AnalogCurrentManager* analogCurrentMgr;
    DigitalIOManager* digitalIOMgr;
    ModbusManager* modbusMgr;
    MultiRTUModbusManager* multiRTUModbusMgr;
    
    // System state
    bool initialized;
    bool autoUpdateEnabled;
    unsigned long lastUpdateTime;
    unsigned long globalUpdateInterval;
    
    // Statistics
    unsigned long totalReadings;
    unsigned long successfulReadings;
    unsigned long failedReadings;
    unsigned long totalSensors;
    unsigned long activeSensors;
    
    // Alarm management
    std::vector<UnifiedAlarmEvent> activeAlarms;
    std::vector<UnifiedAlarmEvent> alarmHistory;
    size_t maxAlarmHistory;
    bool alarmEnabled;
    
    // Callbacks
    void (*sensorUpdatedCallback)(const UnifiedSensorReading& reading);
    void (*alarmTriggeredCallback)(const UnifiedAlarmEvent& alarm);
    void (*sensorStatusChangedCallback)(const String& sensorId, UnifiedSensorStatus oldStatus, UnifiedSensorStatus newStatus);
    
    // Internal methods
    bool readDigitalSensor(const UnifiedSensorConfig& config, UnifiedSensorReading& reading);
    bool readAnalogVoltageSensor(const UnifiedSensorConfig& config, UnifiedSensorReading& reading);
    bool readAnalogCurrentSensor(const UnifiedSensorConfig& config, UnifiedSensorReading& reading);
    bool readModbusSensor(const UnifiedSensorConfig& config, UnifiedSensorReading& reading);
    bool readVirtualSensor(const UnifiedSensorConfig& config, UnifiedSensorReading& reading);
    bool readSystemSensor(const UnifiedSensorConfig& config, UnifiedSensorReading& reading);
    
    void processReading(const UnifiedSensorConfig& config, UnifiedSensorReading& reading);
    void checkAlarms(const UnifiedSensorConfig& config, const UnifiedSensorReading& reading);
    void triggerAlarm(const UnifiedSensorConfig& config, const UnifiedSensorReading& reading, 
                     const String& alarmType, float threshold);
    void updateSensorStatus(const String& sensorId, UnifiedSensorStatus status);
    void logReading(const UnifiedSensorReading& reading);
    void streamReading(const UnifiedSensorReading& reading);
    size_t getSensorIndex(const String& sensorId);
    String generateSensorId(const UnifiedSensorConfig& config);
    
public:
    // Constructor and destructor
    UnifiedSensorManager();
    ~UnifiedSensorManager();
    
    // System management
    bool begin();
    void end();
    void update();
    bool restart();
    bool isInitialized();
    
    // Manager assignment
    void setAnalogVoltageManager(AnalogVoltageManager* mgr);
    void setAnalogCurrentManager(AnalogCurrentManager* mgr);
    void setDigitalIOManager(DigitalIOManager* mgr);
    void setModbusManager(ModbusManager* mgr);
    void setMultiRTUModbusManager(MultiRTUModbusManager* mgr);
    
    // Sensor configuration
    bool addSensor(const UnifiedSensorConfig& config);
    bool removeSensor(const String& sensorId);
    bool updateSensorConfig(const String& sensorId, const UnifiedSensorConfig& newConfig);
    UnifiedSensorConfig* getSensorConfig(const String& sensorId);
    std::vector<UnifiedSensorConfig> getAllSensorConfigs();
    std::vector<UnifiedSensorConfig> getSensorsByType(UnifiedSensorType type);
    std::vector<UnifiedSensorConfig> getSensorsByGroup(const String& group);
    
    // Sensor control
    bool enableSensor(const String& sensorId, bool enable = true);
    bool enableSensorGroup(const String& group, bool enable = true);
    bool enableAllSensors(bool enable = true);
    bool calibrateSensor(const String& sensorId, float referenceValue);
    bool resetSensorCalibration(const String& sensorId);
    
    // Data access
    UnifiedSensorReading getLastReading(const String& sensorId);
    std::vector<UnifiedSensorReading> getAllLastReadings();
    std::vector<UnifiedSensorReading> getReadingsByType(UnifiedSensorType type);
    std::vector<UnifiedSensorReading> getReadingsByGroup(const String& group);
    bool readSensor(const String& sensorId, UnifiedSensorReading& reading);
    bool readAllSensors();
    
    // Digital output control (for actuators)
    bool setDigitalOutput(const String& sensorId, bool state);
    bool setDigitalOutputMode(const String& sensorId, const String& mode, float value = 0);
    
    // Modbus write operations
    bool writeModbusRegister(const String& sensorId, float value);
    
    // Update control
    void enableAutoUpdate(bool enable = true);
    void setGlobalUpdateInterval(unsigned long interval);
    void setSensorUpdateInterval(const String& sensorId, unsigned long interval);
    
    // Alarm management
    void enableAlarms(bool enable = true);
    std::vector<UnifiedAlarmEvent> getActiveAlarms();
    std::vector<UnifiedAlarmEvent> getAlarmHistory(unsigned long since = 0);
    bool acknowledgeAlarm(size_t alarmIndex, const String& acknowledgedBy = "System");
    bool clearAlarm(size_t alarmIndex);
    void clearAllAlarms();
    void clearAlarmHistory();
    
    // Statistics
    struct SystemStats {
        unsigned long totalSensors;
        unsigned long activeSensors;
        unsigned long totalReadings;
        unsigned long successfulReadings;
        unsigned long failedReadings;
        float successRate;
        unsigned long uptime;
        unsigned long lastUpdateTime;
        String systemStatus;
    };
    SystemStats getSystemStats();
    void resetStatistics();
    
    // Configuration management
    bool exportConfiguration(String& jsonConfig);
    bool importConfiguration(const String& jsonConfig);
    bool saveConfigurationToSD(const String& filename = "/unified_sensors.json");
    bool loadConfigurationFromSD(const String& filename = "/unified_sensors.json");
    
    // Advanced sensor creation helpers
    String addAnalogVoltageSensor(const String& name, uint8_t channel, 
                                 float minVoltage = 0.0, float maxVoltage = 10.0,
                                 const String& unit = "V", const String& group = "voltage");
    String addAnalogCurrentSensor(const String& name, uint8_t channel,
                                 float minCurrent = 4.0, float maxCurrent = 20.0,
                                 const String& unit = "mA", const String& group = "current");
    String addDigitalInputSensor(const String& name, uint8_t pin,
                                const String& group = "digital");
    String addDigitalOutputSensor(const String& name, uint8_t pin,
                                 const String& group = "digital");
    String addModbusSensor(const String& name, uint8_t slaveId, const String& registerName,
                          const String& unit = "", const String& group = "modbus");
    String addMultiRTUModbusSensor(const String& name, uint8_t interfaceId, uint8_t slaveId,
                                  const String& registerName, const String& unit = "",
                                  const String& group = "modbus");
    
    // Bulk operations
    bool enableSensorsByType(UnifiedSensorType type, bool enable = true);
    bool readSensorsByType(UnifiedSensorType type);
    bool readSensorsByGroup(const String& group);
    
    // Search and filtering
    std::vector<String> findSensorsByTag(const String& tagKey, const String& tagValue = "");
    std::vector<String> findSensorsByLocation(const String& location);
    std::vector<String> findSensorsWithAlarms();
    std::vector<String> findOfflineSensors();
    
    // Callbacks
    void setSensorUpdatedCallback(void (*callback)(const UnifiedSensorReading&));
    void setAlarmTriggeredCallback(void (*callback)(const UnifiedAlarmEvent&));
    void setSensorStatusChangedCallback(void (*callback)(const String&, UnifiedSensorStatus, UnifiedSensorStatus));
    
    // Diagnostics
    String getDiagnosticReport();
    bool testSensor(const String& sensorId);
    bool testAllSensors();
    std::vector<String> getHealthReport();
};

// Global instance
extern UnifiedSensorManager unifiedSensors;