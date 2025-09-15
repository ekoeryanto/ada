#ifndef MODBUS_MANAGER_H
#define MODBUS_MANAGER_H

#include <Arduino.h>
#include <ModbusMaster.h>
#include <HardwareSerial.h>
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

// Modbus device configuration
struct ModbusDeviceConfig {
    String name;                    // Device name
    String description;             // Device description
    String manufacturer;            // Device manufacturer
    String model;                   // Device model
    String serialNumber;            // Device serial number
    String firmwareVersion;         // Firmware version
    
    uint8_t slaveId;               // Modbus slave ID
    uint32_t baudRate;             // Communication baud rate
    uint8_t dataBits;              // Data bits (7 or 8)
    uint8_t parity;                // Parity (0=None, 1=Odd, 2=Even)
    uint8_t stopBits;              // Stop bits (1 or 2)
    
    bool enabled;                   // Device enabled flag
    String group;                   // Device group
    String tags;                    // Device tags
    
    // Timing configuration
    unsigned long responseTimeout; // Response timeout (ms)
    unsigned long frameDelay;      // Inter-frame delay (ms)
    unsigned long retryDelay;      // Retry delay (ms)
    uint8_t maxRetries;            // Maximum retries
    
    // Health monitoring
    bool healthMonitoring;         // Enable health monitoring
    unsigned long healthInterval;  // Health check interval (ms)
    
    // Register maps
    std::vector<ModbusRegisterMap> inputRegisters;    // Input registers (read-only)
    std::vector<ModbusRegisterMap> holdingRegisters;  // Holding registers (read/write)
    std::vector<ModbusRegisterMap> coils;             // Coils (read/write bits)
    std::vector<ModbusRegisterMap> discreteInputs;    // Discrete inputs (read-only bits)
};

// Modbus device status
struct ModbusDeviceStatus {
    bool connected;                // Connection status
    bool responding;               // Response status
    unsigned long lastCommunication; // Last successful communication
    unsigned long totalRequests;  // Total requests sent
    unsigned long successfulRequests; // Successful requests
    unsigned long failedRequests;  // Failed requests
    unsigned long timeoutCount;    // Timeout count
    unsigned long errorCount;      // Error count
    float successRate;             // Success rate percentage
    float responseTime;            // Average response time (ms)
    uint8_t lastError;             // Last Modbus error code
    String lastErrorMessage;       // Last error message
    unsigned long connectionTime;  // Total connection time
    bool autoDiscovered;           // Auto-discovered device
    float healthScore;             // Health score (0-100%)
};

// Modbus data reading result
struct ModbusReading {
    String deviceName;             // Device name
    String registerName;           // Register name
    uint16_t address;              // Register address
    ModbusDataType dataType;       // Data type
    
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

// Modbus Manager Class
class ModbusManager {
private:
    // Hardware configuration
    HardwareSerial* serial;         // Serial interface
    ModbusMaster* master;           // Modbus master instance
    uint8_t rxPin;                  // RX pin
    uint8_t txPin;                  // TX pin
    uint8_t dePin;                  // DE/RE pin (optional)
    uint8_t slaveAddress;           // Our own Modbus slave address (for future use)
    bool slaveEnabled;              // Enable slave functionality (disabled for now)
    
    // Slave data arrays
    uint16_t holdingRegs[100];      // Holding registers (0-99)
    uint16_t inputRegs[100];        // Input registers (0-99)
    bool coils[100];                // Coils (0-99)
    bool discreteInputs[100];       // Discrete inputs (0-99)
    
    // Device management
    std::vector<ModbusDeviceConfig> devices;
    std::map<uint8_t, size_t> deviceMap; // slaveId -> device index
    size_t currentDevice;           // Current device index for scanning
    
    // Status and statistics
    std::vector<ModbusDeviceStatus> deviceStatus;
    ModbusNetworkStats networkStats;
    
    // Timing and control
    bool initialized;
    bool scanningEnabled;
    bool autoDiscoveryEnabled;
    unsigned long lastScanTime;
    unsigned long scanInterval;
    unsigned long lastDiscoveryTime;
    unsigned long discoveryInterval;
    unsigned long lastStatsUpdate;
    unsigned long statsInterval;
    
    // Data storage
    std::map<String, ModbusReading> lastReadings; // deviceName:registerName -> reading
    std::vector<ModbusReading> readingBuffer;     // Recent readings buffer
    size_t maxBufferSize;
    
    // Logging and streaming
    bool loggingEnabled;
    bool streamingEnabled;
    unsigned long lastLogTime;
    unsigned long logInterval;
    unsigned long lastStreamTime;
    unsigned long streamInterval;
    
    // Callbacks
    void (*deviceConnectedCallback)(uint8_t slaveId, const String& deviceName);
    void (*deviceDisconnectedCallback)(uint8_t slaveId, const String& deviceName);
    void (*dataUpdatedCallback)(const ModbusReading& reading);
    void (*errorCallback)(uint8_t slaveId, uint8_t errorCode, const String& message);
    
    // Internal methods
    void scanDevices();
    void scanDevice(size_t deviceIndex);
    void readRegisters(size_t deviceIndex, ModbusFunctionCode function, 
                      std::vector<ModbusRegisterMap>& registers);
    bool readSingleRegister(size_t deviceIndex, ModbusRegisterMap& regMap, 
                           ModbusFunctionCode function);
    
    void updateDeviceHealth(size_t deviceIndex);
    void updateNetworkStats();
    void processReading(const ModbusReading& reading);
    
    // Auto-discovery methods
    void performAutoDiscovery();
    bool pingDevice(uint8_t slaveId);
    void identifyDevice(uint8_t slaveId);
    
    // Data conversion methods
    float convertToFloat(const std::vector<uint16_t>& data, ModbusDataType type, 
                        ModbusByteOrder byteOrder, float scale, float offset);
    String convertToString(const std::vector<uint16_t>& data, size_t length);
    uint32_t convertToUint32(const std::vector<uint16_t>& data, ModbusByteOrder byteOrder);
    int32_t convertToInt32(const std::vector<uint16_t>& data, ModbusByteOrder byteOrder);
    
    // Error handling
    void handleModbusError(uint8_t result, size_t deviceIndex, const String& operation);
    String getErrorMessage(uint8_t errorCode);
    
    // Logging and streaming
    void logModbusData();
    void generateStreamEvent();
    
public:
    ModbusManager();
    ~ModbusManager();
    
    // Initialization and configuration
    bool begin(uint8_t rxPin = RS485_RX, uint8_t txPin = RS485_TX, uint8_t dePin = RS485_DE);
    bool begin(HardwareSerial* customSerial, uint8_t dePin = RS485_DE);
    void end();
    
    // Device management
    bool addDevice(const ModbusDeviceConfig& config);
    bool removeDevice(uint8_t slaveId);
    bool enableDevice(uint8_t slaveId, bool enable = true);
    bool updateDevice(uint8_t slaveId, const ModbusDeviceConfig& config);
    
    // Register mapping
    bool addRegister(uint8_t slaveId, const ModbusRegisterMap& regMap, ModbusFunctionCode function);
    bool removeRegister(uint8_t slaveId, const String& registerName);
    bool updateRegister(uint8_t slaveId, const String& registerName, const ModbusRegisterMap& regMap);
    
    // Communication
    ModbusReading readRegister(uint8_t slaveId, const String& registerName);
    bool writeRegister(uint8_t slaveId, const String& registerName, float value);
    bool writeCoil(uint8_t slaveId, const String& coilName, bool value);
    
    // Bulk operations
    std::vector<ModbusReading> readAllRegisters(uint8_t slaveId);
    std::vector<ModbusReading> readDeviceGroup(const String& groupName);
    bool writeMultipleRegisters(uint8_t slaveId, const std::map<String, float>& values);
    
    // Auto-discovery
    void enableAutoDiscovery(bool enable = true);
    void setDiscoveryRange(uint8_t startId, uint8_t endId);
    std::vector<uint8_t> scanForDevices();
    
    // Configuration
    void setScanInterval(unsigned long interval);
    void setResponseTimeout(unsigned long timeout);
    void setRetryConfiguration(uint8_t maxRetries, unsigned long retryDelay);
    void enableHealthMonitoring(bool enable = true);
    
    // Data access
    ModbusReading getLastReading(uint8_t slaveId, const String& registerName);
    ModbusReading getLastReading(const String& deviceName, const String& registerName);
    std::vector<ModbusReading> getRecentReadings(size_t count = 100);
    std::vector<ModbusReading> getDeviceReadings(uint8_t slaveId);
    
    // Device information
    ModbusDeviceConfig getDeviceConfig(uint8_t slaveId);
    ModbusDeviceStatus getDeviceStatus(uint8_t slaveId);
    std::vector<uint8_t> getConnectedDevices();
    std::vector<String> getDeviceNames();
    
    // Network status
    ModbusNetworkStats getNetworkStats();
    String getNetworkInfo();
    bool isNetworkHealthy();
    float getNetworkLoad();
    
    // Control methods
    void handle();  // Main loop handler
    void startScanning();
    void stopScanning();
    void triggerScan();
    void resetNetworkStats();
    void resetDeviceStats(uint8_t slaveId);
    
    // Logging and streaming
    void enableLogging(bool enable = true);
    void setLogInterval(unsigned long interval);
    void enableStreaming(bool enable = true);
    void setStreamInterval(unsigned long interval);
    
    // Callbacks
    void setDeviceConnectedCallback(void (*callback)(uint8_t, const String&));
    void setDeviceDisconnectedCallback(void (*callback)(uint8_t, const String&));
    void setDataUpdatedCallback(void (*callback)(const ModbusReading&));
    void setErrorCallback(void (*callback)(uint8_t, uint8_t, const String&));
    
    // Utility methods
    bool isInitialized();
    bool isDeviceConnected(uint8_t slaveId);
    String getDeviceName(uint8_t slaveId);
    uint8_t getDeviceCount();
    
    // JSON export/import
    String exportConfiguration();
    bool importConfiguration(const String& jsonConfig);
    String getStatusJSON();
    String getReadingsJSON();
    
    // Diagnostics
    bool performDiagnostics();
    String getDiagnosticReport();
    void calibrateTimings();
    
    // Advanced features
    void setAdvancedTiming(unsigned long charTimeout, unsigned long frameTimeout);
    void enableFlowControl(bool enable = true);
    void setBaudRate(uint32_t baudRate);
    void setSerialConfig(uint8_t dataBits, uint8_t parity, uint8_t stopBits);
    
    // Additional methods for PLC-like functionality
    bool deviceExists(uint8_t slaveId);
    uint8_t getConnectedDeviceCount();
    bool startDeviceDiscovery(uint8_t startId = 1, uint8_t endId = 247);
    unsigned long getLastCommunicationTime(uint8_t slaveId);
    uint8_t getDeviceHealth(uint8_t slaveId);
    uint16_t getErrorCount(uint8_t slaveId);
    bool writeRegister(uint8_t slaveId, uint16_t address, uint16_t value);
    
    // Modbus Master functionality for communication with other devices
    bool enableMaster(uint8_t rxPin = RS485_RX, uint8_t txPin = RS485_TX, uint8_t dePin = RS485_DE);
    void disableMaster();
    bool isMasterEnabled();
    
    // Future: Modbus Slave functionality (not implemented yet)
    // bool enableSlave(uint8_t slaveAddr = MODBUS_SLAVE_ADDRESS);
    // void disableSlave();
    // bool isSlaveEnabled();
    // void updateSlaveData();
};

// Global instance
extern ModbusManager modbusManager;

// Helper macros for common register types
#define MODBUS_TEMP_HUMIDITY_SHT20(slaveId, tempAddr, humiAddr) \
    ModbusRegisterMap{ \
        "Temperature", tempAddr, MB_TYPE_UINT16, MB_BYTE_ORDER_ABCD, 1, \
        0.1f, 0.0f, "°C", "Environment", "temperature,sht20", true, \
        -40.0f, 85.0f, true, 5000, 0, false, 0, 0 \
    }, \
    ModbusRegisterMap{ \
        "Humidity", humiAddr, MB_TYPE_UINT16, MB_BYTE_ORDER_ABCD, 1, \
        0.1f, 0.0f, "%RH", "Environment", "humidity,sht20", true, \
        0.0f, 100.0f, true, 5000, 0, false, 0, 0 \
    }

#define MODBUS_ENERGY_METER_BASIC(slaveId, voltageAddr, currentAddr, powerAddr) \
    ModbusRegisterMap{ \
        "Voltage", voltageAddr, MB_TYPE_FLOAT32, MB_BYTE_ORDER_ABCD, 2, \
        1.0f, 0.0f, "V", "Power", "voltage,energy", true, \
        0.0f, 1000.0f, true, 2000, 0, false, 0, 0 \
    }, \
    ModbusRegisterMap{ \
        "Current", currentAddr, MB_TYPE_FLOAT32, MB_BYTE_ORDER_ABCD, 2, \
        1.0f, 0.0f, "A", "Power", "current,energy", true, \
        0.0f, 100.0f, true, 2000, 0, false, 0, 0 \
    }, \
    ModbusRegisterMap{ \
        "Power", powerAddr, MB_TYPE_FLOAT32, MB_BYTE_ORDER_ABCD, 2, \
        1.0f, 0.0f, "W", "Power", "power,energy", true, \
        0.0f, 10000.0f, true, 2000, 0, false, 0, 0 \
    }

#endif // MODBUS_MANAGER_H