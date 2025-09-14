#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <ModbusMaster.h>
#include <vector>
#include <map>
#include "config.h"
#include "pins_config.h"

// Forward declarations
class AnalyticsManager;
class WebServerHandler;
class WebhookHandler;

// Modbus function codes
enum ModbusFunctionCode {
    MB_READ_COILS = 0x01,
    MB_READ_DISCRETE_INPUTS = 0x02,
    MB_READ_HOLDING_REGISTERS = 0x03,
    MB_READ_INPUT_REGISTERS = 0x04,
    MB_WRITE_SINGLE_COIL = 0x05,
    MB_WRITE_SINGLE_REGISTER = 0x06,
    MB_WRITE_MULTIPLE_COILS = 0x0F,
    MB_WRITE_MULTIPLE_REGISTERS = 0x10
};

// Modbus data types
enum ModbusDataType {
    MB_TYPE_BOOL,           // Single bit
    MB_TYPE_UINT16,         // 16-bit unsigned integer
    MB_TYPE_INT16,          // 16-bit signed integer
    MB_TYPE_UINT32,         // 32-bit unsigned integer (2 registers)
    MB_TYPE_INT32,          // 32-bit signed integer (2 registers)
    MB_TYPE_FLOAT32,        // 32-bit float (2 registers)
    MB_TYPE_FLOAT64,        // 64-bit double (4 registers)
    MB_TYPE_STRING,         // ASCII string (multiple registers)
    MB_TYPE_RAW             // Raw register data
};

// Modbus byte order for multi-register values
enum ModbusByteOrder {
    MB_BYTE_ORDER_ABCD,     // Big endian (Motorola)
    MB_BYTE_ORDER_DCBA,     // Little endian (Intel)
    MB_BYTE_ORDER_BADC,     // Mid-big endian
    MB_BYTE_ORDER_CDAB      // Mid-little endian
};

// Modbus register types
enum ModbusRegisterType {
    MB_REG_INPUT,           // Input registers (read-only)
    MB_REG_HOLDING,         // Holding registers (read/write)
    MB_REG_COIL,            // Coils (read/write bits)
    MB_REG_DISCRETE         // Discrete inputs (read-only bits)
};

// Modbus register mapping
struct ModbusRegisterMap {
    String name;                    // Register name/description
    uint16_t address;               // Register address
    ModbusDataType dataType;        // Data type
    ModbusByteOrder byteOrder;      // Byte order for multi-register
    uint16_t registerCount;         // Number of registers
    float scaleFactor;              // Scaling factor
    float offset;                   // Offset value
    String unit;                    // Unit of measurement
    String group;                   // Register group
    String tags;                    // Tags for filtering
    bool readOnly;                  // Read-only flag
    
    // Value constraints
    float minValue;                 // Minimum allowed value
    float maxValue;                 // Maximum allowed value
    
    // Update configuration
    bool autoUpdate;                // Auto-update flag
    unsigned long updateInterval;   // Update interval (ms)
    unsigned long lastUpdate;       // Last update time
    
    // Quality indicators
    bool valid;                     // Data validity
    uint8_t quality;                // Data quality (0-100%)
    unsigned long timestamp;        // Last read timestamp
};

// Modbus data reading result
struct ModbusReading {
    String deviceName;             // Device name
    String registerName;           // Register name
    uint16_t address;              // Register address
    ModbusDataType dataType;       // Data type
    uint8_t interfaceId;           // RTU interface ID
    
    // Raw data
    std::vector<uint16_t> rawData; // Raw register data
    
    // Processed data
    union {
        bool boolValue;
        uint16_t uint16Value;
        int16_t int16Value;
        uint32_t uint32Value;
        int32_t int32Value;
        float floatValue;
        double doubleValue;
    } value;
    
    String stringValue;            // String representation
    float scaledValue;             // Scaled value
    String unit;                   // Unit of measurement
    
    // Quality indicators
    bool valid;                    // Data validity
    uint8_t quality;               // Data quality (0-100%)
    unsigned long timestamp;       // Reading timestamp
    unsigned long responseTime;    // Response time (ms)
    uint8_t errorCode;             // Modbus error code (0 = success)
    String errorMessage;           // Error message
};

// Modbus network statistics
struct ModbusNetworkStats {
    unsigned long totalDevices;     // Total configured devices
    unsigned long activeDevices;    // Currently active devices
    unsigned long totalRegisters;   // Total registers configured
    unsigned long totalRequests;    // Total requests sent
    unsigned long successfulRequests; // Successful requests
    unsigned long failedRequests;   // Failed requests
    float networkSuccessRate;       // Network success rate
    float averageResponseTime;      // Average response time
    unsigned long networkUptime;    // Network uptime
    unsigned long lastScanTime;     // Last complete scan time
    float networkLoad;              // Network load percentage
    String networkStatus;           // Network status message
};

// Multi-RTU Configuration
struct ModbusRTUInterface {
    uint8_t interfaceId;           // RTU interface ID (0, 1, 2, ...)
    String name;                   // Interface name (e.g., "RTU_A", "RTU_B")
    HardwareSerial* serial;        // Serial interface pointer
    ModbusMaster* master;          // Modbus master instance
    
    // Pin configuration
    uint8_t rxPin;                 // RX pin
    uint8_t txPin;                 // TX pin
    uint8_t dePin;                 // DE/RE pin (optional, 255 = not used)
    
    // Serial configuration
    uint32_t baudRate;             // Baud rate (default: 9600)
    uint32_t serialConfig;         // Serial config (default: SERIAL_8N1)
    
    // Timing configuration
    unsigned long responseTimeout; // Response timeout (ms)
    unsigned long frameDelay;      // Inter-frame delay (ms)
    unsigned long retryDelay;      // Retry delay (ms)
    uint8_t maxRetries;            // Maximum retries
    
    // Status
    bool initialized;              // Interface initialization status
    bool enabled;                  // Interface enable/disable
    unsigned long lastActivity;    // Last communication timestamp
    unsigned long totalRequests;   // Total requests sent
    unsigned long successfulRequests; // Successful requests
    unsigned long failedRequests;   // Failed requests
    
    // Default constructor
    ModbusRTUInterface() : 
        interfaceId(0), name("RTU_0"), serial(nullptr), master(nullptr),
        rxPin(255), txPin(255), dePin(255),
        baudRate(9600), serialConfig(SERIAL_8N1),
        responseTimeout(1000), frameDelay(10), retryDelay(100), maxRetries(3),
        initialized(false), enabled(true), lastActivity(0),
        totalRequests(0), successfulRequests(0), failedRequests(0) {}
};

// Extended device configuration with RTU interface assignment
struct ModbusDeviceConfigEx {
    uint8_t slaveId;               // Slave ID (1-247)
    uint8_t rtuInterface;          // RTU interface ID (0, 1, 2, ...)
    String name;                   // Device name
    String description;            // Device description
    String deviceType;             // Device type identifier
    String firmwareVersion;        // Firmware version
    
    // Communication parameters
    uint32_t baudRate;             // Baud rate override (0 = use interface default)
    unsigned long scanInterval;    // Individual scan interval (ms)
    unsigned long responseTimeout; // Response timeout override
    unsigned long frameDelay;      // Inter-frame delay override
    unsigned long retryDelay;      // Retry delay override
    uint8_t maxRetries;            // Maximum retries override
    
    // Health monitoring
    bool healthMonitoring;         // Enable health monitoring
    unsigned long healthInterval;  // Health check interval (ms)
    
    // Register maps (same as before)
    std::vector<ModbusRegisterMap> inputRegisters;
    std::vector<ModbusRegisterMap> holdingRegisters;
    std::vector<ModbusRegisterMap> coils;
    std::vector<ModbusRegisterMap> discreteInputs;
    
    // Priority and grouping
    uint8_t priority;              // Device priority (0-255, 0 = highest)
    String group;                  // Device group for batch operations
    
    // Auto-discovery
    bool autoDiscovered;           // Was this device auto-discovered?
    unsigned long discoveryTime;   // When was it discovered
    
    // Default constructor
    ModbusDeviceConfigEx() :
        slaveId(1), rtuInterface(0), name("Device"), description(""),
        deviceType("Generic"), firmwareVersion("Unknown"),
        baudRate(0), scanInterval(1000), responseTimeout(0),
        frameDelay(0), retryDelay(0), maxRetries(0),
        healthMonitoring(true), healthInterval(30000),
        priority(128), group("default"), autoDiscovered(false), discoveryTime(0) {}
};

// Multi-RTU Modbus Manager
class MultiRTUModbusManager {
private:
    // RTU Interfaces
    std::vector<ModbusRTUInterface> rtuInterfaces;
    std::map<uint8_t, size_t> interfaceMap; // interfaceId -> interface index
    
    // Devices across all RTU interfaces
    std::vector<ModbusDeviceConfigEx> devices;
    std::map<String, size_t> deviceMap; // "interfaceId:slaveId" -> device index
    
    // Scanning and scheduling
    size_t currentInterfaceIndex;   // Current interface being scanned
    size_t currentDeviceIndex;      // Current device being scanned
    unsigned long lastGlobalScan;   // Last global scan timestamp
    unsigned long globalScanInterval; // Global scan interval
    
    // Statistics per interface
    std::map<uint8_t, ModbusNetworkStats> interfaceStats;
    
    // Global settings
    bool initialized;
    bool globalScanningEnabled;
    bool autoDiscoveryEnabled;
    unsigned long discoveryInterval;
    unsigned long lastDiscoveryTime;
    
    // Data management
    std::map<String, ModbusReading> lastReadings; // "interface:device:register" -> reading
    std::vector<ModbusReading> readingBuffer;
    size_t maxBufferSize;
    
    // Logging and streaming
    bool loggingEnabled;
    bool streamingEnabled;
    unsigned long logInterval;
    unsigned long streamInterval;
    unsigned long lastLogTime;
    unsigned long lastStreamTime;
    
    // Callbacks
    void (*deviceConnectedCallback)(uint8_t interfaceId, uint8_t slaveId, const String& deviceName);
    void (*deviceDisconnectedCallback)(uint8_t interfaceId, uint8_t slaveId, const String& deviceName);
    void (*dataUpdatedCallback)(const ModbusReading& reading);
    void (*errorCallback)(uint8_t interfaceId, uint8_t slaveId, uint8_t errorCode, const String& message);
    void (*interfaceStatusCallback)(uint8_t interfaceId, bool connected, const String& status);
    
    // Internal methods
    bool initializeInterface(size_t interfaceIndex);
    void scanInterface(size_t interfaceIndex);
    void scanDevice(size_t deviceIndex);
    bool readDeviceRegisters(size_t deviceIndex);
    void updateDeviceStatus(size_t deviceIndex, bool success);
    void handleModbusError(uint8_t result, size_t deviceIndex, const String& operation);
    void logReading(const ModbusReading& reading);
    void streamReading(const ModbusReading& reading);
    void updateStatistics();
    String createDeviceKey(uint8_t interfaceId, uint8_t slaveId);
    size_t getDeviceIndex(uint8_t interfaceId, uint8_t slaveId);
    size_t getInterfaceIndex(uint8_t interfaceId);

public:
    // Constructor
    MultiRTUModbusManager();
    ~MultiRTUModbusManager();
    
    // RTU Interface Management
    bool addRTUInterface(uint8_t interfaceId, const String& name, 
                        uint8_t rxPin, uint8_t txPin, uint8_t dePin = 255,
                        uint32_t baudRate = 9600, uint32_t serialConfig = SERIAL_8N1);
    bool removeRTUInterface(uint8_t interfaceId);
    bool enableRTUInterface(uint8_t interfaceId, bool enable = true);
    bool isRTUInterfaceEnabled(uint8_t interfaceId);
    std::vector<uint8_t> getAvailableRTUInterfaces();
    ModbusNetworkStats getRTUInterfaceStats(uint8_t interfaceId);
    String getRTUInterfaceStatus(uint8_t interfaceId);
    
    // Device Management
    bool addDevice(const ModbusDeviceConfigEx& deviceConfig);
    bool removeDevice(uint8_t interfaceId, uint8_t slaveId);
    bool enableDevice(uint8_t interfaceId, uint8_t slaveId, bool enable = true);
    ModbusDeviceConfigEx* getDeviceConfig(uint8_t interfaceId, uint8_t slaveId);
    std::vector<ModbusDeviceConfigEx> getDevicesOnInterface(uint8_t interfaceId);
    std::vector<ModbusDeviceConfigEx> getAllDevices();
    
    // Register Management
    bool addRegisterMap(uint8_t interfaceId, uint8_t slaveId, const ModbusRegisterMap& registerMap, ModbusRegisterType type);
    bool removeRegisterMap(uint8_t interfaceId, uint8_t slaveId, const String& registerName, ModbusRegisterType type);
    std::vector<ModbusRegisterMap> getRegisterMaps(uint8_t interfaceId, uint8_t slaveId, ModbusRegisterType type);
    
    // System Control
    bool begin();
    void end();
    void update();
    bool restart();
    bool restartInterface(uint8_t interfaceId);
    
    // Scanning Control
    void startScanning();
    void stopScanning();
    bool isScanningEnabled();
    void setScanInterval(unsigned long interval);
    void enableAutoDiscovery(bool enable = true);
    bool scanForDevices(uint8_t interfaceId);
    std::vector<uint8_t> getConnectedDevices(uint8_t interfaceId);
    
    // Data Access
    ModbusReading getLastReading(uint8_t interfaceId, uint8_t slaveId, const String& registerName);
    std::vector<ModbusReading> getLastReadings(uint8_t interfaceId, uint8_t slaveId);
    std::vector<ModbusReading> getAllLastReadings();
    bool readRegister(uint8_t interfaceId, uint8_t slaveId, const String& registerName, ModbusReading& reading);
    bool writeRegister(uint8_t interfaceId, uint8_t slaveId, const String& registerName, float value);
    
    // Logging and Streaming
    void enableLogging(bool enable = true);
    void enableStreaming(bool enable = true);
    void setLogInterval(unsigned long interval);
    void setStreamInterval(unsigned long interval);
    
    // Statistics and Status
    ModbusNetworkStats getGlobalStats();
    std::map<uint8_t, ModbusNetworkStats> getAllInterfaceStats();
    String getSystemStatus();
    bool isInitialized();
    
    // Device Discovery and Configuration
    bool discoverDevicesOnInterface(uint8_t interfaceId);
    bool autoConfigureDevice(uint8_t interfaceId, uint8_t slaveId);
    bool validateDeviceConfiguration(const ModbusDeviceConfigEx& config);
    
    // Bulk Operations
    bool enableAllDevices(bool enable = true);
    bool enableAllDevicesOnInterface(uint8_t interfaceId, bool enable = true);
    bool scanAllInterfaces();
    void resetAllStatistics();
    void resetInterfaceStatistics(uint8_t interfaceId);
    
    // Advanced Features
    bool setDevicePriority(uint8_t interfaceId, uint8_t slaveId, uint8_t priority);
    bool setDeviceGroup(uint8_t interfaceId, uint8_t slaveId, const String& group);
    std::vector<ModbusDeviceConfigEx> getDevicesByGroup(const String& group);
    bool enableDeviceGroup(const String& group, bool enable = true);
    
    // Callbacks
    void setDeviceConnectedCallback(void (*callback)(uint8_t, uint8_t, const String&));
    void setDeviceDisconnectedCallback(void (*callback)(uint8_t, uint8_t, const String&));
    void setDataUpdatedCallback(void (*callback)(const ModbusReading&));
    void setErrorCallback(void (*callback)(uint8_t, uint8_t, uint8_t, const String&));
    void setInterfaceStatusCallback(void (*callback)(uint8_t, bool, const String&));
    
    // Configuration Import/Export
    bool exportConfiguration(String& jsonConfig);
    bool importConfiguration(const String& jsonConfig);
    bool saveConfigurationToSD(const String& filename = "/modbus_config.json");
    bool loadConfigurationFromSD(const String& filename = "/modbus_config.json");
    
    // Diagnostics
    bool testInterface(uint8_t interfaceId);
    bool testDevice(uint8_t interfaceId, uint8_t slaveId);
    bool pingDevice(uint8_t interfaceId, uint8_t slaveId);
    String getDiagnosticReport(uint8_t interfaceId = 255); // 255 = all interfaces
};

// Global instance for multi-RTU Modbus manager
extern MultiRTUModbusManager multiRTUModbus;