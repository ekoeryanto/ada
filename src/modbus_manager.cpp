#include "modbus_manager.h"
#include "sd_manager.h"
#include "ntp_manager.h"
#include "web_server.h"
#include "webhook_handler.h"
#include "analytics_manager.h"
#include <vector>

// Global instance
ModbusManager modbusManager;

ModbusManager::ModbusManager() :
    serial(nullptr),
    master(nullptr),
    rxPin(RS485_RX),
    txPin(RS485_TX),
    dePin(RS485_DE),
    slaveAddress(MODBUS_SLAVE_ADDRESS),
    slaveEnabled(false),
    currentDevice(0),
    initialized(false),
    scanningEnabled(true),
    autoDiscoveryEnabled(false),
    lastScanTime(0),
    scanInterval(5000),      // 5 seconds default scan
    lastDiscoveryTime(0),
    discoveryInterval(300000), // 5 minutes discovery
    lastStatsUpdate(0),
    statsInterval(10000),    // 10 seconds stats update
    maxBufferSize(1000),
    loggingEnabled(true),
    streamingEnabled(true),
    lastLogTime(0),
    logInterval(60000),      // 1 minute logging
    lastStreamTime(0),
    streamInterval(5000),    // 5 seconds streaming
    deviceConnectedCallback(nullptr),
    deviceDisconnectedCallback(nullptr),
    dataUpdatedCallback(nullptr),
    errorCallback(nullptr) {
    
    // Initialize network stats
    networkStats = {
        0, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.0f, "Initializing"
    };
}

ModbusManager::~ModbusManager() {
    end();
}

bool ModbusManager::begin(uint8_t rxPin, uint8_t txPin, uint8_t dePin) {
    if (DEBUG_ENABLED) {
        Serial.println("[Modbus] Initializing Modbus RTU Manager...");
    }
    
    this->rxPin = rxPin;
    this->txPin = txPin;
    this->dePin = dePin;
    
    // Initialize hardware serial
    serial = new HardwareSerial(1);
    if (!serial) {
        Serial.println("[Modbus] Failed to create HardwareSerial instance");
        return false;
    }
    
    serial->begin(19200, SERIAL_8E1, rxPin, txPin);  // Altivar 61: 19200 baud, 8-E-1
    
    // Initialize Modbus master
    master = new ModbusMaster();
    if (!master) {
        Serial.println("[Modbus] Failed to create ModbusMaster instance");
        delete serial;
        serial = nullptr;
        return false;
    }
    
    master->begin(1, *serial);  // Default slave ID 1, will be changed per device
    
    // Configure DE pin if provided
    if (dePin != 255) {
        pinMode(dePin, OUTPUT);
        digitalWrite(dePin, LOW);  // Receive mode by default
        
        // Set pre/post transmission callbacks for DE pin control
        master->preTransmission([]() {
            digitalWrite(modbusManager.dePin, HIGH);  // Transmit mode
            delayMicroseconds(100);  // Small delay for line settling
        });
        
        master->postTransmission([]() {
            delayMicroseconds(100);  // Small delay for transmission completion
            digitalWrite(modbusManager.dePin, LOW);   // Receive mode
        });
    }
    
        // The ModbusMaster library doesn't expose timeout configuration
    // We'll rely on default timeouts
    
    initialized = true;
    networkStats.networkStatus = "Online";
    
    if (DEBUG_ENABLED) {
        Serial.printf("[Modbus] Initialized - RX:%d, TX:%d, DE:%d\n", rxPin, txPin, dePin);
    }
    
    return true;
}

bool ModbusManager::begin(HardwareSerial* customSerial, uint8_t dePin) {
    if (!customSerial) return false;
    
    this->serial = customSerial;
    this->dePin = dePin;
    
    // Initialize Modbus master
    master = new ModbusMaster();
    if (!master) {
        Serial.println("[Modbus] Failed to create ModbusMaster instance");
        return false;
    }
    
    master->begin(1, *serial);
    
    // Configure DE pin if provided
    if (dePin != 255) {
        pinMode(dePin, OUTPUT);
        digitalWrite(dePin, LOW);
        
        master->preTransmission([]() {
            digitalWrite(modbusManager.dePin, HIGH);
            delayMicroseconds(100);
        });
        
        master->postTransmission([]() {
            delayMicroseconds(100);
            digitalWrite(modbusManager.dePin, LOW);
        });
    }
    
    // Use default timeout configuration from library
    initialized = true;
    networkStats.networkStatus = "Online";
    
    if (DEBUG_ENABLED) {
        Serial.println("[Modbus] Initialized with custom serial");
    }
    
    return true;
}

void ModbusManager::end() {
    if (master) {
        delete master;
        master = nullptr;
    }
    
    if (serial) {
        serial->end();
        delete serial;
        serial = nullptr;
    }
    
    devices.clear();
    deviceMap.clear();
    deviceStatus.clear();
    lastReadings.clear();
    readingBuffer.clear();
    
    initialized = false;
    networkStats.networkStatus = "Offline";
    
    if (DEBUG_ENABLED) {
        Serial.println("[Modbus] Manager shutdown complete");
    }
}

bool ModbusManager::addDevice(const ModbusDeviceConfig& config) {
    if (!initialized) return false;
    
    // Check if device already exists
    if (deviceMap.find(config.slaveId) != deviceMap.end()) {
        if (DEBUG_ENABLED) {
            Serial.printf("[Modbus] Device with slave ID %d already exists\n", config.slaveId);
        }
        return false;
    }
    
    // Add device
    devices.push_back(config);
    size_t deviceIndex = devices.size() - 1;
    deviceMap[config.slaveId] = deviceIndex;
    
    // Initialize device status
    ModbusDeviceStatus status = {
        false, false, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0, "", 0, false, 0.0f
    };
    deviceStatus.push_back(status);
    
    networkStats.totalDevices++;
    networkStats.totalRegisters += config.inputRegisters.size() + 
                                  config.holdingRegisters.size() + 
                                  config.coils.size() + 
                                  config.discreteInputs.size();
    
    if (DEBUG_ENABLED) {
        Serial.printf("[Modbus] Added device: %s (ID: %d)\n", 
                     config.name.c_str(), config.slaveId);
    }
    
    return true;
}

void ModbusManager::handle() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // Perform device scanning
    if (scanningEnabled && (currentTime - lastScanTime >= scanInterval)) {
        scanDevices();
        lastScanTime = currentTime;
    }
    
    // Perform auto-discovery
    if (autoDiscoveryEnabled && (currentTime - lastDiscoveryTime >= discoveryInterval)) {
        performAutoDiscovery();
        lastDiscoveryTime = currentTime;
    }
    
    // Update network statistics
    if (currentTime - lastStatsUpdate >= statsInterval) {
        updateNetworkStats();
        lastStatsUpdate = currentTime;
    }
    
    // Data logging
    if (loggingEnabled && (currentTime - lastLogTime >= logInterval)) {
        logModbusData();
        lastLogTime = currentTime;
    }
    
    // Real-time streaming
    if (streamingEnabled && (currentTime - lastStreamTime >= streamInterval)) {
        generateStreamEvent();
        lastStreamTime = currentTime;
    }
}

void ModbusManager::scanDevices() {
    if (devices.empty()) return;
    
    // Scan next device in round-robin fashion
    if (currentDevice >= devices.size()) {
        currentDevice = 0;
    }
    
    if (devices[currentDevice].enabled) {
        scanDevice(currentDevice);
    }
    
    currentDevice++;
    
    // Complete scan cycle
    if (currentDevice >= devices.size()) {
        networkStats.lastScanTime = millis();
        currentDevice = 0;
    }
}

void ModbusManager::scanDevice(size_t deviceIndex) {
    if (deviceIndex >= devices.size()) return;
    
    ModbusDeviceConfig& device = devices[deviceIndex];
    ModbusDeviceStatus& status = deviceStatus[deviceIndex];
    
    unsigned long scanStartTime = millis();
    
    // Set slave ID for this device
    master->begin(device.slaveId, *serial);
    
    // Read input registers
    if (!device.inputRegisters.empty()) {
        readRegisters(deviceIndex, MB_READ_INPUT_REGISTERS, device.inputRegisters);
    }
    
    // Read holding registers
    if (!device.holdingRegisters.empty()) {
        readRegisters(deviceIndex, MB_READ_HOLDING_REGISTERS, device.holdingRegisters);
    }
    
    // Read coils
    if (!device.coils.empty()) {
        readRegisters(deviceIndex, MB_READ_COILS, device.coils);
    }
    
    // Read discrete inputs
    if (!device.discreteInputs.empty()) {
        readRegisters(deviceIndex, MB_READ_DISCRETE_INPUTS, device.discreteInputs);
    }
    
    // Update device health
    updateDeviceHealth(deviceIndex);
    
    unsigned long scanDuration = millis() - scanStartTime;
    status.responseTime = (status.responseTime * 0.9f) + (scanDuration * 0.1f); // Moving average
    
    if (DEBUG_ENABLED && status.totalRequests % 10 == 0) {
        Serial.printf("[Modbus] Device %s scan completed in %lums\n", 
                     device.name.c_str(), scanDuration);
    }
}

void ModbusManager::readRegisters(size_t deviceIndex, ModbusFunctionCode function, 
                                 std::vector<ModbusRegisterMap>& registers) {
    for (auto& regMap : registers) {
        if (!regMap.autoUpdate) continue;
        
        unsigned long currentTime = millis();
        if (currentTime - regMap.lastUpdate < regMap.updateInterval) continue;
        
        if (readSingleRegister(deviceIndex, regMap, function)) {
            regMap.lastUpdate = currentTime;
        }
    }
}

bool ModbusManager::readSingleRegister(size_t deviceIndex, ModbusRegisterMap& regMap, 
                                      ModbusFunctionCode function) {
    if (deviceIndex >= devices.size()) return false;
    
    ModbusDeviceConfig& device = devices[deviceIndex];
    ModbusDeviceStatus& status = deviceStatus[deviceIndex];
    
    unsigned long requestStartTime = millis();
    uint8_t result = 0;
    
    status.totalRequests++;
    networkStats.totalRequests++;
    
    // Execute Modbus command based on function code
    switch (function) {
        case MB_READ_INPUT_REGISTERS:
            result = master->readInputRegisters(regMap.address, regMap.registerCount);
            break;
            
        case MB_READ_HOLDING_REGISTERS:
            result = master->readHoldingRegisters(regMap.address, regMap.registerCount);
            break;
            
        case MB_READ_COILS:
            result = master->readCoils(regMap.address, regMap.registerCount);
            break;
            
        case MB_READ_DISCRETE_INPUTS:
            result = master->readDiscreteInputs(regMap.address, regMap.registerCount);
            break;
            
        default:
            return false;
    }
    
    unsigned long responseTime = millis() - requestStartTime;
    
    if (result == master->ku8MBSuccess) {
        // Successful read
        status.successfulRequests++;
        status.lastCommunication = millis();
        status.connected = true;
        status.responding = true;
        networkStats.successfulRequests++;
        
        // Create reading object
        ModbusReading reading;
        reading.deviceName = device.name;
        reading.registerName = regMap.name;
        reading.address = regMap.address;
        reading.dataType = regMap.dataType;
        reading.unit = regMap.unit;
        reading.timestamp = millis();
        reading.responseTime = responseTime;
        reading.errorCode = 0;
        reading.valid = true;
        reading.quality = 100;  // Full quality for successful read
        
        // Extract raw data
        reading.rawData.clear();
        for (uint16_t i = 0; i < regMap.registerCount; i++) {
            if (function == MB_READ_COILS || function == MB_READ_DISCRETE_INPUTS) {
                // For discrete values, get bit status
                reading.rawData.push_back(master->getResponseBuffer(i) ? 1 : 0);
            } else {
                // For registers, get 16-bit values
                reading.rawData.push_back(master->getResponseBuffer(i));
            }
        }
        
        // Convert to appropriate data type
        reading.scaledValue = convertToFloat(reading.rawData, regMap.dataType, 
                                           regMap.byteOrder, regMap.scaleFactor, regMap.offset);
        
        // Set union value based on data type
        switch (regMap.dataType) {
            case MB_TYPE_BOOL:
                reading.value.boolValue = (reading.rawData[0] != 0);
                reading.stringValue = reading.value.boolValue ? "true" : "false";
                break;
                
            case MB_TYPE_UINT16:
                reading.value.uint16Value = reading.rawData[0];
                reading.stringValue = String(reading.value.uint16Value);
                break;
                
            case MB_TYPE_INT16:
                reading.value.int16Value = (int16_t)reading.rawData[0];
                reading.stringValue = String(reading.value.int16Value);
                break;
                
            case MB_TYPE_UINT32:
                reading.value.uint32Value = convertToUint32(reading.rawData, regMap.byteOrder);
                reading.stringValue = String(reading.value.uint32Value);
                break;
                
            case MB_TYPE_INT32:
                reading.value.int32Value = convertToInt32(reading.rawData, regMap.byteOrder);
                reading.stringValue = String(reading.value.int32Value);
                break;
                
            case MB_TYPE_FLOAT32:
                reading.value.floatValue = reading.scaledValue;
                reading.stringValue = String(reading.value.floatValue, 3);
                break;
                
            case MB_TYPE_STRING:
                reading.stringValue = convertToString(reading.rawData, regMap.registerCount);
                break;
                
            default:
                reading.stringValue = "RAW:" + String(reading.rawData[0]);
                break;
        }
        
        // Update register map
        regMap.valid = true;
        regMap.quality = 100;
        regMap.timestamp = reading.timestamp;
        
        // Store reading
        String readingKey = device.name + ":" + regMap.name;
        lastReadings[readingKey] = reading;
        
        // Add to buffer
        readingBuffer.push_back(reading);
        if (readingBuffer.size() > maxBufferSize) {
            readingBuffer.erase(readingBuffer.begin());
        }
        
        // Process reading
        processReading(reading);
        
        if (DEBUG_ENABLED && status.totalRequests % 20 == 0) {
            Serial.printf("[Modbus] %s:%s = %s %s (%.1fms)\n", 
                         device.name.c_str(), regMap.name.c_str(), 
                         reading.stringValue.c_str(), regMap.unit.c_str(), 
                         (float)responseTime);
        }
        
        return true;
        
    } else {
        // Failed read
        status.failedRequests++;
        status.errorCount++;
        status.lastError = result;
        status.lastErrorMessage = getErrorMessage(result);
        networkStats.failedRequests++;
        
        // Update register map
        regMap.valid = false;
        regMap.quality = 0;
        regMap.timestamp = millis();
        
        handleModbusError(result, deviceIndex, "Read " + regMap.name);
        
        return false;
    }
}

float ModbusManager::convertToFloat(const std::vector<uint16_t>& data, ModbusDataType type, 
                                   ModbusByteOrder byteOrder, float scale, float offset) {
    if (data.empty()) return 0.0f;
    
    float value = 0.0f;
    
    switch (type) {
        case MB_TYPE_BOOL:
            value = (data[0] != 0) ? 1.0f : 0.0f;
            break;
            
        case MB_TYPE_UINT16:
            value = (float)data[0];
            break;
            
        case MB_TYPE_INT16:
            value = (float)(int16_t)data[0];
            break;
            
        case MB_TYPE_UINT32:
            value = (float)convertToUint32(data, byteOrder);
            break;
            
        case MB_TYPE_INT32:
            value = (float)convertToInt32(data, byteOrder);
            break;
            
        case MB_TYPE_FLOAT32:
            if (data.size() >= 2) {
                union {
                    float f;
                    uint32_t i;
                } floatConverter;
                
                floatConverter.i = convertToUint32(data, byteOrder);
                value = floatConverter.f;
            }
            break;
            
        default:
            value = (float)data[0];
            break;
    }
    
    return (value * scale) + offset;
}

uint32_t ModbusManager::convertToUint32(const std::vector<uint16_t>& data, ModbusByteOrder byteOrder) {
    if (data.size() < 2) return 0;
    
    uint32_t result = 0;
    
    switch (byteOrder) {
        case MB_BYTE_ORDER_ABCD:  // Big endian
            result = ((uint32_t)data[0] << 16) | data[1];
            break;
            
        case MB_BYTE_ORDER_DCBA:  // Little endian
            result = ((uint32_t)data[1] << 16) | data[0];
            break;
            
        case MB_BYTE_ORDER_BADC:  // Mid-big endian
            result = (((uint32_t)data[0] & 0xFF) << 24) | (((uint32_t)data[0] & 0xFF00) << 8) |
                    (((uint32_t)data[1] & 0xFF) << 8) | ((uint32_t)data[1] & 0xFF00 >> 8);
            break;
            
        case MB_BYTE_ORDER_CDAB:  // Mid-little endian
            result = (((uint32_t)data[1] & 0xFF) << 24) | (((uint32_t)data[1] & 0xFF00) << 8) |
                    (((uint32_t)data[0] & 0xFF) << 8) | ((uint32_t)data[0] & 0xFF00 >> 8);
            break;
    }
    
    return result;
}

int32_t ModbusManager::convertToInt32(const std::vector<uint16_t>& data, ModbusByteOrder byteOrder) {
    return (int32_t)convertToUint32(data, byteOrder);
}

String ModbusManager::convertToString(const std::vector<uint16_t>& data, size_t length) {
    String result = "";
    
    for (size_t i = 0; i < length && i < data.size(); i++) {
        uint16_t reg = data[i];
        char highByte = (char)((reg >> 8) & 0xFF);
        char lowByte = (char)(reg & 0xFF);
        
        if (highByte != 0) result += highByte;
        if (lowByte != 0) result += lowByte;
    }
    
    return result;
}

void ModbusManager::updateDeviceHealth(size_t deviceIndex) {
    if (deviceIndex >= deviceStatus.size()) return;
    
    ModbusDeviceStatus& status = deviceStatus[deviceIndex];
    
    // Calculate success rate
    if (status.totalRequests > 0) {
        status.successRate = ((float)status.successfulRequests / status.totalRequests) * 100.0f;
    }
    
    // Calculate health score based on multiple factors
    float healthScore = 100.0f;
    
    // Success rate impact (70% weight)
    healthScore *= (status.successRate / 100.0f) * 0.7f;
    
    // Response time impact (20% weight)
    if (status.responseTime > 5000) {
        healthScore *= 0.2f;  // Very slow response
    } else if (status.responseTime > 2000) {
        healthScore *= 0.6f;  // Slow response
    } else if (status.responseTime > 1000) {
        healthScore *= 0.8f;  // Moderate response
    }
    healthScore += 20.0f;  // Base 20% for response time
    
    // Recent communication impact (10% weight)
    unsigned long timeSinceLastComm = millis() - status.lastCommunication;
    if (timeSinceLastComm > 60000) {
        healthScore *= 0.1f;  // No communication for over 1 minute
    } else if (timeSinceLastComm > 30000) {
        healthScore *= 0.5f;  // No communication for over 30 seconds
    } else {
        healthScore += 10.0f;  // Recent communication bonus
    }
    
    status.healthScore = max(0.0f, min(100.0f, healthScore));
    
    // Update connection status
    unsigned long timeSinceComm = millis() - status.lastCommunication;
    status.responding = (timeSinceComm < 30000);  // Consider responding if within 30 seconds
    status.connected = status.responding && (status.successRate > 50.0f);
}

void ModbusManager::updateNetworkStats() {
    networkStats.activeDevices = 0;
    float totalSuccessRate = 0.0f;
    float totalResponseTime = 0.0f;
    
    for (const auto& status : deviceStatus) {
        if (status.connected) {
            networkStats.activeDevices++;
            totalSuccessRate += status.successRate;
            totalResponseTime += status.responseTime;
        }
    }
    
    if (networkStats.activeDevices > 0) {
        networkStats.networkSuccessRate = totalSuccessRate / networkStats.activeDevices;
        networkStats.averageResponseTime = totalResponseTime / networkStats.activeDevices;
    }
    
    if (networkStats.totalRequests > 0) {
        networkStats.networkSuccessRate = ((float)networkStats.successfulRequests / networkStats.totalRequests) * 100.0f;
    }
    
    // Calculate network load (requests per second)
    static unsigned long lastRequestCount = 0;
    static unsigned long lastLoadUpdate = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastLoadUpdate >= 10000) {  // Update every 10 seconds
        unsigned long requestsDiff = networkStats.totalRequests - lastRequestCount;
        unsigned long timeDiff = currentTime - lastLoadUpdate;
        
        networkStats.networkLoad = ((float)requestsDiff / (timeDiff / 1000.0f));
        
        lastRequestCount = networkStats.totalRequests;
        lastLoadUpdate = currentTime;
    }
    
    // Update network status
    if (networkStats.activeDevices == 0) {
        networkStats.networkStatus = "No devices connected";
    } else if (networkStats.networkSuccessRate > 90.0f) {
        networkStats.networkStatus = "Excellent";
    } else if (networkStats.networkSuccessRate > 75.0f) {
        networkStats.networkStatus = "Good";
    } else if (networkStats.networkSuccessRate > 50.0f) {
        networkStats.networkStatus = "Fair";
    } else {
        networkStats.networkStatus = "Poor";
    }
}

void ModbusManager::processReading(const ModbusReading& reading) {
    // Trigger data updated callback
    if (dataUpdatedCallback) {
        dataUpdatedCallback(reading);
    }
    
    // Add to analytics if available
    extern AnalyticsManager analyticsMgr;
    if (analyticsMgr.isInitialized()) {
        // Create unique sensor ID for analytics
        String sensorId = "modbus_" + reading.deviceName + "_" + reading.registerName;
        // Simple hash function for String (Arduino doesn't have hashCode())
        uint32_t hash = 0;
        for (int i = 0; i < sensorId.length(); i++) {
            hash = hash * 31 + sensorId.charAt(i);
        }
        analyticsMgr.addDataPoint(hash % 100, reading.scaledValue, reading.timestamp);
    }
}

void ModbusManager::handleModbusError(uint8_t result, size_t deviceIndex, const String& operation) {
    if (deviceIndex >= devices.size()) return;
    
    ModbusDeviceConfig& device = devices[deviceIndex];
    ModbusDeviceStatus& status = deviceStatus[deviceIndex];
    
    String errorMsg = getErrorMessage(result);
    
    if (DEBUG_ENABLED) {
        Serial.printf("[Modbus] Error in %s for device %s (ID:%d): %s\n", 
                     operation.c_str(), device.name.c_str(), device.slaveId, errorMsg.c_str());
    }
    
    // Handle specific error types
    switch (result) {
        case 0xE0:  // Timeout
            status.timeoutCount++;
            status.responding = false;
            break;
            
        case 0xE1:  // Invalid slave ID
        case 0xE2:  // Invalid function
            status.connected = false;
            break;
            
        default:
            // Other Modbus errors
            break;
    }
    
    // Trigger error callback
    if (errorCallback) {
        errorCallback(device.slaveId, result, errorMsg);
    }
    
    // Send webhook notification for critical errors
    extern WebhookHandler webhookHandler;
    if (webhookHandler.isInitialized() && status.errorCount % 10 == 0) {
        webhookHandler.sendAlarmTriggered(
            "modbus_device_" + String(device.slaveId),
            "MODBUS_ERROR",
            (float)result,
            0.0f
        );
    }
}

String ModbusManager::getErrorMessage(uint8_t errorCode) {
    switch (errorCode) {
        case 0x00: return "Success";
        case 0x01: return "Illegal Function";
        case 0x02: return "Illegal Data Address";
        case 0x03: return "Illegal Data Value";
        case 0x04: return "Slave Device Failure";
        case 0x05: return "Acknowledge";
        case 0x06: return "Slave Device Busy";
        case 0x08: return "Memory Parity Error";
        case 0x0A: return "Gateway Path Unavailable";
        case 0x0B: return "Gateway Target Failed to Respond";
        case 0xE0: return "Timeout";
        case 0xE1: return "Invalid Slave ID";
        case 0xE2: return "Invalid Function";
        case 0xE3: return "Response Length Mismatch";
        case 0xE4: return "Invalid CRC";
        default: return "Unknown Error (" + String(errorCode, HEX) + ")";
    }
}

void ModbusManager::performAutoDiscovery() {
    if (!autoDiscoveryEnabled) return;
    
    if (DEBUG_ENABLED) {
        Serial.println("[Modbus] Starting auto-discovery...");
    }
    
    std::vector<uint8_t> foundDevices;
    
    // Scan for devices (typically 1-247 for Modbus RTU)
    for (uint8_t slaveId = 1; slaveId <= 10; slaveId++) {  // Limited range for demo
        if (deviceMap.find(slaveId) != deviceMap.end()) {
            continue;  // Skip already configured devices
        }
        
        if (pingDevice(slaveId)) {
            foundDevices.push_back(slaveId);
            identifyDevice(slaveId);
        }
        
        delay(100);  // Small delay between discovery attempts
    }
    
    if (DEBUG_ENABLED) {
        Serial.printf("[Modbus] Auto-discovery completed. Found %d new devices\n", foundDevices.size());
    }
}

bool ModbusManager::pingDevice(uint8_t slaveId) {
    master->begin(slaveId, *serial);
    
    // Try to read a common register (holding register 0)
    uint8_t result = master->readHoldingRegisters(0, 1);
    
    return (result == master->ku8MBSuccess);
}

void ModbusManager::identifyDevice(uint8_t slaveId) {
    // Create a basic device configuration for discovered device
    ModbusDeviceConfig config;
    config.name = "Auto_Device_" + String(slaveId);
    config.description = "Auto-discovered Modbus device";
    config.slaveId = slaveId;
    config.baudRate = 9600;
    config.dataBits = 8;
    config.parity = 0;
    config.stopBits = 1;
    config.enabled = true;
    config.group = "Auto-Discovered";
    config.tags = "auto,discovered";
    config.responseTimeout = 1000;
    config.frameDelay = 100;
    config.retryDelay = 500;
    config.maxRetries = 3;
    config.healthMonitoring = true;
    config.healthInterval = 30000;
    
    // Add some common registers to try
    ModbusRegisterMap regMap;
    regMap.name = "Register_0";
    regMap.address = 0;
    regMap.dataType = MB_TYPE_UINT16;
    regMap.byteOrder = MB_BYTE_ORDER_ABCD;
    regMap.registerCount = 1;
    regMap.scaleFactor = 1.0f;
    regMap.offset = 0.0f;
    regMap.unit = "";
    regMap.group = "General";
    regMap.tags = "auto";
    regMap.readOnly = true;
    regMap.autoUpdate = true;
    regMap.updateInterval = 5000;
    regMap.valid = false;
    regMap.quality = 0;
    regMap.timestamp = 0;
    
    config.holdingRegisters.push_back(regMap);
    
    // Add the device
    if (addDevice(config)) {
        if (DEBUG_ENABLED) {
            Serial.printf("[Modbus] Auto-discovered device added: %s (ID: %d)\n", 
                         config.name.c_str(), slaveId);
        }
        
        // Mark as auto-discovered
        deviceStatus[deviceMap[slaveId]].autoDiscovered = true;
        
        // Trigger callback
        if (deviceConnectedCallback) {
            deviceConnectedCallback(slaveId, config.name);
        }
    }
}

void ModbusManager::logModbusData() {
    if (!loggingEnabled || !sdMgr.isMounted()) return;
    
    String logEntry = "";
    
    for (const auto& reading : lastReadings) {
        if (logEntry.length() > 0) logEntry += ",";
        
        logEntry += reading.second.deviceName + ":" + reading.second.registerName + "=" + 
                   String(reading.second.scaledValue, 3) + reading.second.unit;
    }
    
    if (logEntry.length() > 0) {
        if (sdMgr.logDataWithTimestamp("MODBUS," + logEntry)) {
            if (DEBUG_ENABLED) {
                Serial.println("[Modbus] Data logged: " + logEntry);
            }
        }
    }
}

void ModbusManager::generateStreamEvent() {
    if (!streamingEnabled) return;
    
    // Generate JSON for WebSocket streaming
    String streamData = "{";
    streamData += "\"type\":\"modbus\",";
    streamData += "\"timestamp\":" + String(millis()) + ",";
    streamData += "\"network\":{";
    streamData += "\"status\":\"" + networkStats.networkStatus + "\",";
    streamData += "\"devices\":" + String(networkStats.activeDevices) + ",";
    streamData += "\"successRate\":" + String(networkStats.networkSuccessRate, 1) + ",";
    streamData += "\"responseTime\":" + String(networkStats.averageResponseTime, 1);
    streamData += "},";
    
    streamData += "\"devices\":[";
    bool first = true;
    for (size_t i = 0; i < devices.size(); i++) {
        if (!first) streamData += ",";
        first = false;
        
        streamData += "{";
        streamData += "\"name\":\"" + devices[i].name + "\",";
        streamData += "\"slaveId\":" + String(devices[i].slaveId) + ",";
        streamData += "\"connected\":" + String(deviceStatus[i].connected ? "true" : "false") + ",";
        streamData += "\"health\":" + String(deviceStatus[i].healthScore, 1) + ",";
        streamData += "\"successRate\":" + String(deviceStatus[i].successRate, 1);
        streamData += "}";
    }
    streamData += "],";
    
    streamData += "\"readings\":[";
    first = true;
    for (const auto& reading : lastReadings) {
        if (!first) streamData += ",";
        first = false;
        
        streamData += "{";
        streamData += "\"device\":\"" + reading.second.deviceName + "\",";
        streamData += "\"register\":\"" + reading.second.registerName + "\",";
        streamData += "\"value\":" + String(reading.second.scaledValue, 3) + ",";
        streamData += "\"unit\":\"" + reading.second.unit + "\",";
        streamData += "\"quality\":" + String(reading.second.quality);
        streamData += "}";
    }
    streamData += "]";
    streamData += "}";
    
    // Broadcast to WebSocket clients
    extern WebServerHandler webServer;
    webServer.broadcastToWebSocket(streamData);
}

// Public interface methods implementation (simplified for space)
ModbusReading ModbusManager::readRegister(uint8_t slaveId, const String& registerName) {
    auto deviceIt = deviceMap.find(slaveId);
    if (deviceIt == deviceMap.end()) {
        return ModbusReading(); // Return empty reading
    }
    
    String readingKey = devices[deviceIt->second].name + ":" + registerName;
    auto readingIt = lastReadings.find(readingKey);
    
    if (readingIt != lastReadings.end()) {
        return readingIt->second;
    }
    
    return ModbusReading(); // Return empty reading
}

bool ModbusManager::writeRegister(uint8_t slaveId, const String& registerName, float value) {
    auto deviceIt = deviceMap.find(slaveId);
    if (deviceIt == deviceMap.end()) return false;
    
    size_t deviceIndex = deviceIt->second;
    ModbusDeviceConfig& device = devices[deviceIndex];
    
    // Find register in holding registers
    for (auto& regMap : device.holdingRegisters) {
        if (regMap.name == registerName && !regMap.readOnly) {
            master->begin(slaveId, *serial);
            
            // Convert float value to register value
            uint16_t regValue = (uint16_t)((value - regMap.offset) / regMap.scaleFactor);
            
            uint8_t result = master->writeSingleRegister(regMap.address, regValue);
            
            if (result == master->ku8MBSuccess) {
                if (DEBUG_ENABLED) {
                    Serial.printf("[Modbus] Write successful: %s:%s = %.3f\n", 
                                 device.name.c_str(), registerName.c_str(), value);
                }
                return true;
            } else {
                handleModbusError(result, deviceIndex, "Write " + registerName);
                return false;
            }
        }
    }
    
    return false; // Register not found or read-only
}

// Remaining public methods (simplified implementations)
void ModbusManager::setScanInterval(unsigned long interval) { scanInterval = max(interval, 1000UL); }
void ModbusManager::enableAutoDiscovery(bool enable) { autoDiscoveryEnabled = enable; }
void ModbusManager::startScanning() { scanningEnabled = true; }
void ModbusManager::stopScanning() { scanningEnabled = false; }
void ModbusManager::enableLogging(bool enable) { loggingEnabled = enable; }
void ModbusManager::enableStreaming(bool enable) { streamingEnabled = enable; }
void ModbusManager::setLogInterval(unsigned long interval) { logInterval = max(interval, 1000UL); }
void ModbusManager::setStreamInterval(unsigned long interval) { streamInterval = max(interval, 1000UL); }
bool ModbusManager::isInitialized() { return initialized; }
ModbusNetworkStats ModbusManager::getNetworkStats() { return networkStats; }

// Callback setters
void ModbusManager::setDeviceConnectedCallback(void (*callback)(uint8_t, const String&)) {
    deviceConnectedCallback = callback;
}

void ModbusManager::setDeviceDisconnectedCallback(void (*callback)(uint8_t, const String&)) {
    deviceDisconnectedCallback = callback;
}

void ModbusManager::setDataUpdatedCallback(void (*callback)(const ModbusReading&)) {
    dataUpdatedCallback = callback;
}

void ModbusManager::setErrorCallback(void (*callback)(uint8_t, uint8_t, const String&)) {
    errorCallback = callback;
}

void ModbusManager::enableHealthMonitoring(bool enabled) {
    // Update all device configurations
    for (auto& device : devices) {
        device.healthMonitoring = enabled;
    }
    if (DEBUG_ENABLED) {
        Serial.printf("[Modbus] Health monitoring %s\n", enabled ? "enabled" : "disabled");
    }
}

std::vector<uint8_t> ModbusManager::getConnectedDevices() {
    std::vector<uint8_t> connectedIds;
    for (size_t i = 0; i < devices.size(); i++) {
        if (i < deviceStatus.size() && deviceStatus[i].connected) {
            connectedIds.push_back(devices[i].slaveId);
        }
    }
    return connectedIds;
}

String ModbusManager::getDeviceName(uint8_t slaveId) {
    for (const auto& device : devices) {
        if (device.slaveId == slaveId) {
            return device.name;
        }
    }
    return "Unknown Device";
}

// Additional methods for PLC-like functionality
bool ModbusManager::deviceExists(uint8_t slaveId) {
    for (const auto& device : devices) {
        if (device.slaveId == slaveId) {
            return true;
        }
    }
    return false;
}

uint8_t ModbusManager::getConnectedDeviceCount() {
    uint8_t count = 0;
    for (size_t i = 0; i < devices.size(); i++) {
        if (i < deviceStatus.size() && deviceStatus[i].connected) {
            count++;
        }
    }
    return count;
}

bool ModbusManager::startDeviceDiscovery(uint8_t startId, uint8_t endId) {
    if (!initialized || !master) {
        return false;
    }
    
    if (DEBUG_ENABLED) {
        Serial.printf("[Modbus] Starting device discovery: ID %d to %d\n", startId, endId);
    }
    
    // Simple discovery implementation - try to read holding register 0 from each device
    bool foundAny = false;
    for (uint8_t id = startId; id <= endId; id++) {
        if (deviceExists(id)) {
            continue; // Skip already configured devices
        }
        
        // Try to read a common register (usually holding register 0)
        master->clearResponseBuffer();
        uint8_t result = master->readHoldingRegisters(0, 1);
        
        delay(100); // Small delay between attempts
        
        if (result == master->ku8MBSuccess) {
            if (DEBUG_ENABLED) {
                Serial.printf("[Modbus] Device found at ID %d\n", id);
            }
            
            // Create a basic device configuration for discovered device
            ModbusDeviceConfig discoveredDevice;
            discoveredDevice.name = "Auto_Device_" + String(id);
            discoveredDevice.description = "Auto-discovered device";
            discoveredDevice.slaveId = id;
            discoveredDevice.enabled = false; // Disabled by default until configured
            discoveredDevice.baudRate = 9600;
            discoveredDevice.dataBits = 8;
            discoveredDevice.parity = 0;
            discoveredDevice.stopBits = 1;
            discoveredDevice.responseTimeout = 1000;
            discoveredDevice.frameDelay = 100;
            discoveredDevice.retryDelay = 500;
            discoveredDevice.maxRetries = 3;
            discoveredDevice.healthMonitoring = true;
            discoveredDevice.healthInterval = 30000;
            
            addDevice(discoveredDevice);
            foundAny = true;
        }
    }
    
    return foundAny;
}

unsigned long ModbusManager::getLastCommunicationTime(uint8_t slaveId) {
    for (size_t i = 0; i < devices.size(); i++) {
        if (devices[i].slaveId == slaveId && i < deviceStatus.size()) {
            return deviceStatus[i].lastCommunication;
        }
    }
    return 0;
}

uint8_t ModbusManager::getDeviceHealth(uint8_t slaveId) {
    for (size_t i = 0; i < devices.size(); i++) {
        if (devices[i].slaveId == slaveId && i < deviceStatus.size()) {
            // Calculate health score based on success rate and response time
            float successRate = deviceStatus[i].successfulRequests > 0 ?
                (float)deviceStatus[i].successfulRequests / (deviceStatus[i].successfulRequests + deviceStatus[i].errorCount) * 100 : 0;            if (successRate >= 95) return 100; // Excellent
            else if (successRate >= 85) return 80; // Good
            else if (successRate >= 70) return 60; // Fair
            else if (successRate >= 50) return 40; // Poor
            else return 20; // Very poor
        }
    }
    return 0; // Device not found
}

uint16_t ModbusManager::getErrorCount(uint8_t slaveId) {
    for (size_t i = 0; i < devices.size(); i++) {
        if (devices[i].slaveId == slaveId && i < deviceStatus.size()) {
            return deviceStatus[i].errorCount;
        }
    }
    return 0;
}

bool ModbusManager::writeRegister(uint8_t slaveId, uint16_t address, uint16_t value) {
    if (!initialized || !master) {
        return false;
    }
    
    if (!deviceExists(slaveId)) {
        if (DEBUG_ENABLED) {
            Serial.printf("[Modbus] Write failed: Device ID %d not configured\n", slaveId);
        }
        return false;
    }
    
    master->clearResponseBuffer();
    master->begin(slaveId, *serial);
    uint8_t result = master->writeSingleRegister(address, value);
    
    bool success = (result == master->ku8MBSuccess);
    
    if (DEBUG_ENABLED) {
        Serial.printf("[Modbus] Write register %d at device %d: %s (value: %d)\n", 
                     address, slaveId, success ? "SUCCESS" : "FAILED", value);
    }
    
    // Update device statistics
    for (size_t i = 0; i < devices.size(); i++) {
        if (devices[i].slaveId == slaveId && i < deviceStatus.size()) {
            if (success) {
                deviceStatus[i].lastCommunication = millis();
                deviceStatus[i].connected = true;
            } else {
                deviceStatus[i].errorCount++;
            }
            break;
        }
    }
    
    return success;
}

// Missing methods implementation
uint8_t ModbusManager::getDeviceCount() {
    return devices.size();
}

bool ModbusManager::isDeviceConnected(uint8_t slaveId) {
    for (size_t i = 0; i < devices.size(); i++) {
        if (devices[i].slaveId == slaveId && i < deviceStatus.size()) {
            return deviceStatus[i].connected;
        }
    }
    return false;
}

bool ModbusManager::removeDevice(uint8_t slaveId) {
    for (size_t i = 0; i < devices.size(); i++) {
        if (devices[i].slaveId == slaveId) {
            devices.erase(devices.begin() + i);
            if (i < deviceStatus.size()) {
                deviceStatus.erase(deviceStatus.begin() + i);
            }
            if (DEBUG_ENABLED) {
                Serial.printf("[Modbus] Device ID %d removed\n", slaveId);
            }
            return true;
        }
    }
    return false;
}

// Master functionality methods (for future expansion)
bool ModbusManager::enableMaster(uint8_t rxPin, uint8_t txPin, uint8_t dePin) {
    return begin(rxPin, txPin, dePin);
}

void ModbusManager::disableMaster() {
    end();
}

bool ModbusManager::isMasterEnabled() {
    return initialized && master != nullptr;
}