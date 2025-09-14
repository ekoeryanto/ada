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
#include "digital_io_manager.h"
#include "modbus_manager.h"
#include "analytics_manager.h"
#include "remote_diagnostics.h"
#include "webhook_handler.h"

// Global variables for timing
unsigned long lastStatusUpdate = 0;
unsigned long lastWiFiCheck = 0;

// Modbus Event Callbacks
void onModbusDeviceConnected(uint8_t slaveId, const String& deviceName) {
    Serial.printf("[Modbus Event] Device connected: %s (ID: %d)\n", deviceName.c_str(), slaveId);
    
    // Log connection event
    if (sdMgr.isMounted()) {
        String logEntry = "MODBUS_CONNECT," + deviceName + "," + String(slaveId);
        sdMgr.logDataWithTimestamp("EVENT," + logEntry);
    }
    
    // Send webhook notification
    webhookHandler.sendSensorData("modbus_device_" + String(slaveId), 1.0, "connected", millis());
}

void onModbusDeviceDisconnected(uint8_t slaveId, const String& deviceName) {
    Serial.printf("[Modbus Event] Device disconnected: %s (ID: %d)\n", deviceName.c_str(), slaveId);
    
    // Log disconnection event
    if (sdMgr.isMounted()) {
        String logEntry = "MODBUS_DISCONNECT," + deviceName + "," + String(slaveId);
        sdMgr.logDataWithTimestamp("EVENT," + logEntry);
    }
    
    // Send webhook notification
    webhookHandler.sendSensorData("modbus_device_" + String(slaveId), 0.0, "disconnected", millis());
}

void onModbusDataUpdated(const ModbusReading& reading) {
    if (DEBUG_ENABLED && random(100) < 5) {  // Log 5% of readings to avoid spam
        Serial.printf("[Modbus Event] Data: %s:%s = %s %s (Q:%d%%)\n", 
                     reading.deviceName.c_str(), reading.registerName.c_str(),
                     reading.stringValue.c_str(), reading.unit.c_str(), reading.quality);
    }
    
    // Process high-priority readings immediately
    if (reading.deviceName.indexOf("SHT20") >= 0) {
        // Temperature/humidity sensor - check for alarms
        if (reading.registerName == "Temperature" && reading.scaledValue > 40.0) {
            Serial.println("[ALARM] High temperature detected: " + String(reading.scaledValue) + "°C");
            webhookHandler.sendAlarmTriggered("temperature_high", "WARNING", reading.scaledValue, 40.0);
        }
        
        if (reading.registerName == "Humidity" && reading.scaledValue > 80.0) {
            Serial.println("[ALARM] High humidity detected: " + String(reading.scaledValue) + "%RH");
            webhookHandler.sendAlarmTriggered("humidity_high", "WARNING", reading.scaledValue, 80.0);
        }
    }
}

void onModbusError(uint8_t slaveId, uint8_t errorCode, const String& message) {
    Serial.printf("[Modbus Error] Device ID %d: [0x%02X] %s\n", slaveId, errorCode, message.c_str());
    
    // Log error events
    if (sdMgr.isMounted()) {
        String logEntry = "MODBUS_ERROR," + String(slaveId) + "," + String(errorCode, HEX) + "," + message;
        sdMgr.logDataWithTimestamp("ERROR," + logEntry);
    }
    
    // Send critical error notifications
    if (errorCode == 0xE0) {  // Timeout error
        static unsigned long lastTimeoutNotification = 0;
        if (millis() - lastTimeoutNotification > 60000) {  // Max 1 notification per minute
            webhookHandler.sendAlarmTriggered("modbus_timeout", "ERROR", (float)slaveId, 0.0);
            lastTimeoutNotification = millis();
        }
    }
}

// Digital IO Event Callbacks
void onDigitalInputEvent(int inputIndex, DigitalInputEvent event) {
    String inputName = digitalIOMgr.getInputName(inputIndex);
    String eventStr = (event == DI_RISING_EDGE) ? "RISING" : 
                     (event == DI_FALLING_EDGE) ? "FALLING" : "UNKNOWN";
    
    Serial.printf("[DIO Event] Input %d (%s): %s edge detected\n", 
                 inputIndex, inputName.c_str(), eventStr.c_str());
    
    // Handle specific input events
    switch (inputIndex) {
        case 0: // Emergency Stop
            if (event == DI_FALLING_EDGE) {
                Serial.println("[SAFETY] Emergency stop activated - shutting down equipment!");
                // Emergency shutdown procedure
                digitalIOMgr.setOutput(0, false);  // Stop main pump
                digitalIOMgr.setOutput(2, false);  // Close solenoid valve
                digitalIOMgr.setOutputState(1, DO_BLINK);  // Start alarm beacon blinking
                
                // Send emergency webhook
                webhookHandler.sendAlarmTriggered("emergency_stop", "EMERGENCY", 0.0, 1.0);
            }
            break;
            
        case 1: // Tank High Level
            if (event == DI_RISING_EDGE) {
                Serial.println("[LEVEL] Tank high level detected - activating safety measures");
                digitalIOMgr.setOutput(0, false);  // Stop pump to prevent overflow
                digitalIOMgr.setOutput(3, true);   // Turn on status light
            } else if (event == DI_FALLING_EDGE) {
                Serial.println("[LEVEL] Tank level normal - resuming operation");
                digitalIOMgr.setOutput(3, false);  // Turn off status light
            }
            break;
            
        case 2: // Pump Status (pulse counting)
            Serial.printf("[EQUIPMENT] Pump pulse detected - total: %lu\n", 
                         digitalIOMgr.getInputPulseCount(2));
            break;
            
        case 3: // Door Status
            if (event == DI_RISING_EDGE) {
                Serial.println("[SECURITY] Door opened - logging access");
                digitalIOMgr.pulseOutput(3, 2000);  // Pulse status light for 2 seconds
            } else if (event == DI_FALLING_EDGE) {
                Serial.println("[SECURITY] Door closed");
            }
            break;
    }
}

void onDigitalOutputEvent(int outputIndex, DigitalOutputState state) {
    String outputName = digitalIOMgr.getOutputName(outputIndex);
    String stateStr = (state == DO_ON) ? "ON" : 
                     (state == DO_OFF) ? "OFF" : 
                     (state == DO_PWM) ? "PWM" : 
                     (state == DO_PULSE) ? "PULSE" : 
                     (state == DO_BLINK) ? "BLINK" : "UNKNOWN";
    
    Serial.printf("[DIO Event] Output %d (%s): State changed to %s\n", 
                 outputIndex, outputName.c_str(), stateStr.c_str());
    
    // Log output state changes for audit trail
    if (sdMgr.isMounted()) {
        String logEntry = "OUTPUT_" + String(outputIndex) + "," + outputName + "," + stateStr;
        sdMgr.logDataWithTimestamp("EVENT," + logEntry);
    }
}

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
    
    // Initialize Digital IO Manager
    Serial.println("[Main] Initializing Digital IO Manager...");
    if (digitalIOMgr.begin()) {
        Serial.println("[Main] Digital IO Manager initialized successfully");
        
        // Configure digital inputs with specific names and functions
        digitalIOMgr.configureInput(0, "Emergency Stop", "Safety", false, 50);
        digitalIOMgr.configureInput(1, "Tank High Level", "Level", false, 100);
        digitalIOMgr.configureInput(2, "Pump Status", "Equipment", false, 30);
        digitalIOMgr.configureInput(3, "Door Status", "Security", true, 75);  // Inverted logic
        
        // Configure digital outputs with specific names and functions
        digitalIOMgr.configureOutput(0, "Main Pump", "Equipment", false, DO_OFF);
        digitalIOMgr.configureOutput(1, "Alarm Beacon", "Safety", false, DO_OFF);
        digitalIOMgr.configureOutput(2, "Solenoid Valve", "Process", false, DO_OFF);
        digitalIOMgr.configureOutput(3, "Status Light", "Indication", false, DO_OFF);
        
        // Enable pulse counting for pump status monitoring
        digitalIOMgr.enableInputCounting(2, true, 300000);  // Auto-reset every 5 minutes
        
        // Configure input alarms for safety monitoring
        digitalIOMgr.enableInputAlarm(0, DI_LOW, 0, "EMERGENCY STOP ACTIVATED!");
        digitalIOMgr.enableInputAlarm(1, DI_HIGH, 2000, "Tank high level detected");
        digitalIOMgr.enableInputAlarm(3, DI_HIGH, 5000, "Unauthorized door access");
        
        // Set update intervals for optimal performance
        digitalIOMgr.setUpdateInterval(10);     // 10ms for responsive DI/DO
        digitalIOMgr.setLogInterval(60000);     // 1 minute data logging
        digitalIOMgr.setStreamInterval(1000);   // 1 second real-time streaming
        
        // Enable logging and streaming
        digitalIOMgr.enableLogging(sdMgr.isMounted());
        digitalIOMgr.enableStreaming(true);
        
        // Set event callbacks for real-time processing
        digitalIOMgr.setInputEventCallback(onDigitalInputEvent);
        digitalIOMgr.setOutputStateCallback(onDigitalOutputEvent);
        
        Serial.println("[Main] Digital I/O configured:");
        Serial.println("[Main] - DI1: Emergency Stop (Safety)");
        Serial.println("[Main] - DI2: Tank High Level (Level monitoring)");
        Serial.println("[Main] - DI3: Pump Status (Equipment monitoring with pulse counting)");
        Serial.println("[Main] - DI4: Door Status (Security - inverted logic)");
        Serial.println("[Main] - DO1: Main Pump (Equipment control)");
        Serial.println("[Main] - DO2: Alarm Beacon (Safety indication)");
        Serial.println("[Main] - DO3: Solenoid Valve (Process control)");
        Serial.println("[Main] - DO4: Status Light (System indication)");
        Serial.println("[Main] - Safety alarms configured for emergency stop and level monitoring");
    } else {
        Serial.println("[Main] Digital IO Manager initialization failed!");
    }
    
    // Initialize Modbus RTU Manager
    Serial.println("[Main] Initializing Modbus RTU Manager...");
    if (modbusManager.begin()) {
        Serial.println("[Main] Modbus RTU Manager initialized successfully");
        
        // Configure SHT20 Temperature/Humidity sensor (from sample code)
        ModbusDeviceConfig sht20Config;
        sht20Config.name = "SHT20_Sensor";
        sht20Config.description = "Temperature and Humidity Sensor";
        sht20Config.manufacturer = "Sensirion";
        sht20Config.model = "SHT20";
        sht20Config.slaveId = 1;  // From sample code
        sht20Config.baudRate = 9600;
        sht20Config.dataBits = 8;
        sht20Config.parity = 0;  // None
        sht20Config.stopBits = 1;
        sht20Config.enabled = true;
        sht20Config.group = "Environment";
        sht20Config.tags = "temperature,humidity,environment";
        sht20Config.responseTimeout = 1000;
        sht20Config.frameDelay = 100;
        sht20Config.retryDelay = 500;
        sht20Config.maxRetries = 3;
        sht20Config.healthMonitoring = true;
        sht20Config.healthInterval = 30000;
        
        // Temperature register (address 0x001, from sample code)
        ModbusRegisterMap tempReg;
        tempReg.name = "Temperature";
        tempReg.address = 0x001;
        tempReg.dataType = MB_TYPE_UINT16;
        tempReg.byteOrder = MB_BYTE_ORDER_ABCD;
        tempReg.registerCount = 1;
        tempReg.scaleFactor = 0.1f;  // Divide by 10 (from sample code)
        tempReg.offset = 0.0f;
        tempReg.unit = "°C";
        tempReg.group = "Environment";
        tempReg.tags = "temperature,sht20";
        tempReg.readOnly = true;
        tempReg.minValue = -40.0f;
        tempReg.maxValue = 85.0f;
        tempReg.autoUpdate = true;
        tempReg.updateInterval = 5000;  // 5 seconds
        tempReg.valid = false;
        tempReg.quality = 0;
        tempReg.timestamp = 0;
        
        // Humidity register (address 0x002, inferred from sample code logic)
        ModbusRegisterMap humiReg;
        humiReg.name = "Humidity";
        humiReg.address = 0x002;
        humiReg.dataType = MB_TYPE_UINT16;
        humiReg.byteOrder = MB_BYTE_ORDER_ABCD;
        humiReg.registerCount = 1;
        humiReg.scaleFactor = 0.1f;  // Divide by 10
        humiReg.offset = 0.0f;
        humiReg.unit = "%RH";
        humiReg.group = "Environment";
        humiReg.tags = "humidity,sht20";
        humiReg.readOnly = true;
        humiReg.minValue = 0.0f;
        humiReg.maxValue = 100.0f;
        humiReg.autoUpdate = true;
        humiReg.updateInterval = 5000;
        humiReg.valid = false;
        humiReg.quality = 0;
        humiReg.timestamp = 0;
        
        // Add registers to device configuration
        sht20Config.inputRegisters.push_back(tempReg);
        sht20Config.inputRegisters.push_back(humiReg);
        
        // Add SHT20 device
        if (modbusManager.addDevice(sht20Config)) {
            Serial.println("[Main] SHT20 sensor configured successfully");
        } else {
            Serial.println("[Main] Failed to configure SHT20 sensor");
        }
        
        // Configure additional demo device (Energy Meter example)
        ModbusDeviceConfig energyMeterConfig;
        energyMeterConfig.name = "Energy_Meter";
        energyMeterConfig.description = "3-Phase Energy Meter";
        energyMeterConfig.manufacturer = "Generic";
        energyMeterConfig.model = "EM-3000";
        energyMeterConfig.slaveId = 2;
        energyMeterConfig.baudRate = 9600;
        energyMeterConfig.dataBits = 8;
        energyMeterConfig.parity = 0;
        energyMeterConfig.stopBits = 1;
        energyMeterConfig.enabled = false;  // Disabled by default (may not be connected)
        energyMeterConfig.group = "Power";
        energyMeterConfig.tags = "energy,power,meter";
        energyMeterConfig.responseTimeout = 2000;
        energyMeterConfig.frameDelay = 100;
        energyMeterConfig.retryDelay = 1000;
        energyMeterConfig.maxRetries = 3;
        energyMeterConfig.healthMonitoring = true;
        energyMeterConfig.healthInterval = 60000;
        
        // Voltage register
        ModbusRegisterMap voltageReg;
        voltageReg.name = "Voltage";
        voltageReg.address = 0x100;
        voltageReg.dataType = MB_TYPE_FLOAT32;
        voltageReg.byteOrder = MB_BYTE_ORDER_ABCD;
        voltageReg.registerCount = 2;
        voltageReg.scaleFactor = 1.0f;
        voltageReg.offset = 0.0f;
        voltageReg.unit = "V";
        voltageReg.group = "Power";
        voltageReg.tags = "voltage,power";
        voltageReg.readOnly = true;
        voltageReg.minValue = 0.0f;
        voltageReg.maxValue = 1000.0f;
        voltageReg.autoUpdate = true;
        voltageReg.updateInterval = 10000;
        voltageReg.valid = false;
        voltageReg.quality = 0;
        voltageReg.timestamp = 0;
        
        energyMeterConfig.inputRegisters.push_back(voltageReg);
        
        // Add energy meter device (disabled by default)
        modbusManager.addDevice(energyMeterConfig);
        
        // Set Modbus event callbacks
        modbusManager.setDeviceConnectedCallback(onModbusDeviceConnected);
        modbusManager.setDeviceDisconnectedCallback(onModbusDeviceDisconnected);
        modbusManager.setDataUpdatedCallback(onModbusDataUpdated);
        modbusManager.setErrorCallback(onModbusError);
        
        // Configure Modbus settings
        modbusManager.setScanInterval(5000);        // 5 seconds scan cycle
        modbusManager.enableHealthMonitoring(true);
        modbusManager.enableLogging(sdMgr.isMounted());
        modbusManager.enableStreaming(true);
        modbusManager.setLogInterval(300000);       // 5 minutes logging
        modbusManager.setStreamInterval(10000);     // 10 seconds streaming
        
        // Enable auto-discovery for additional devices
        modbusManager.enableAutoDiscovery(true);
        
        Serial.println("[Main] Modbus devices configured:");
        Serial.println("[Main] - SHT20 Temperature/Humidity Sensor (ID: 1) - ENABLED");
        Serial.println("[Main] - Energy Meter Demo (ID: 2) - DISABLED");
        Serial.println("[Main] - Auto-discovery enabled for additional devices");
        Serial.println("[Main] - Health monitoring and logging enabled");
    } else {
        Serial.println("[Main] Modbus RTU Manager initialization failed!");
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
    
    // Handle Digital IO Manager (digital inputs/outputs)
    digitalIOMgr.handle();
    
    // Handle Modbus RTU Manager (RS485 communication)
    modbusManager.handle();
    
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
        
        // Feed digital input data to analytics (offset sensor IDs by 6)
        for (int i = 0; i < 4; i++) {
            DigitalInputReading diReading = digitalIOMgr.getInputReading(i);
            if (diReading.valid) {
                // Convert digital state to numerical value for analytics
                float digitalValue = (diReading.currentState == DI_HIGH) ? 1.0 : 0.0;
                analyticsMgr.addDataPoint(i + 6, digitalValue, diReading.timestamp);
                
                // Also track pulse counts for counting-enabled inputs
                if (digitalIOMgr.getInputPulseCount(i) > 0) {
                    analyticsMgr.addDataPoint(i + 10, (float)digitalIOMgr.getInputPulseCount(i), diReading.timestamp);
                }
            }
        }
        
        // Feed Modbus data to analytics (offset sensor IDs by 14)
        ModbusNetworkStats modbusStats = modbusManager.getNetworkStats();
        if (modbusStats.activeDevices > 0) {
            // Network health metrics
            analyticsMgr.addDataPoint(14, modbusStats.networkSuccessRate, millis());
            analyticsMgr.addDataPoint(15, modbusStats.averageResponseTime, millis());
            
            // Individual device readings
            std::vector<uint8_t> connectedDevices = modbusManager.getConnectedDevices();
            for (uint8_t slaveId : connectedDevices) {
                // Get SHT20 readings if available
                ModbusReading tempReading = modbusManager.readRegister(slaveId, "Temperature");
                if (tempReading.valid) {
                    analyticsMgr.addDataPoint(16 + slaveId, tempReading.scaledValue, tempReading.timestamp);
                }
                
                ModbusReading humiReading = modbusManager.readRegister(slaveId, "Humidity");
                if (humiReading.valid) {
                    analyticsMgr.addDataPoint(20 + slaveId, humiReading.scaledValue, humiReading.timestamp);
                }
            }
        }
        lastAnalyticsUpdate = millis();
    }
    
    // Send periodic sensor data via webhooks
    static unsigned long lastWebhookUpdate = 0;
    if (millis() - lastWebhookUpdate > 30000) {  // Every 30 seconds
        // Send analog voltage sensor data
        for (int i = 0; i < 3; i++) {
            if (analogVoltageMgr.isSensorEnabled(i)) {
                AnalogReading reading = analogVoltageMgr.getReading(i);
                if (reading.valid) {
                    String unit = analogVoltageMgr.getUnit(i);
                    webhookHandler.sendSensorData(
                        "voltage_sensor_" + String(i), 
                        reading.scaledValue, 
                        unit, 
                        reading.timestamp
                    );
                }
            }
        }
        
        // Send analog current sensor data
        for (int i = 0; i < 3; i++) {
            if (analogCurrentMgr.isSensorEnabled(i)) {
                CurrentReading reading = analogCurrentMgr.getReading(i);
                if (reading.valid) {
                    String unit = analogCurrentMgr.getUnit(i);
                    webhookHandler.sendSensorData(
                        "current_sensor_" + String(i), 
                        reading.scaledValue, 
                        unit, 
                        reading.timestamp
                    );
                }
            }
        }
        
        // Send digital input status
        for (int i = 0; i < 4; i++) {
            DigitalInputReading diReading = digitalIOMgr.getInputReading(i);
            if (diReading.valid) {
                webhookHandler.sendSensorData(
                    "digital_input_" + String(i), 
                    (diReading.currentState == DI_HIGH) ? 1.0 : 0.0, 
                    "state", 
                    diReading.timestamp
                );
                
                // Send pulse count data for counting-enabled inputs
                if (digitalIOMgr.getInputPulseCount(i) > 0) {
                    webhookHandler.sendSensorData(
                        "pulse_count_" + String(i), 
                        (float)digitalIOMgr.getInputPulseCount(i), 
                        "pulses", 
                        diReading.timestamp
                    );
                }
            }
        }
        
        // Send Modbus sensor data
        std::vector<uint8_t> connectedDevices = modbusManager.getConnectedDevices();
        for (uint8_t slaveId : connectedDevices) {
            String deviceName = modbusManager.getDeviceName(slaveId);
            
            // Send temperature data
            ModbusReading tempReading = modbusManager.readRegister(slaveId, "Temperature");
            if (tempReading.valid) {
                webhookHandler.sendSensorData(
                    "modbus_temp_" + String(slaveId), 
                    tempReading.scaledValue, 
                    tempReading.unit, 
                    tempReading.timestamp
                );
            }
            
            // Send humidity data
            ModbusReading humiReading = modbusManager.readRegister(slaveId, "Humidity");
            if (humiReading.valid) {
                webhookHandler.sendSensorData(
                    "modbus_humi_" + String(slaveId), 
                    humiReading.scaledValue, 
                    humiReading.unit, 
                    humiReading.timestamp
                );
            }
        }
        
        // Send Modbus network health
        ModbusNetworkStats modbusStats = modbusManager.getNetworkStats();
        if (modbusStats.totalRequests > 0) {
            webhookHandler.sendSensorData(
                "modbus_network_health", 
                modbusStats.networkSuccessRate, 
                "%", 
                millis()
            );
        }
        lastWebhookUpdate = millis();
    }
    
    // Handle WiFi connection monitoring
    if (millis() - lastWiFiCheck > 5000) { // Check every 5 seconds
        wifiMgr.handleWiFi();
        lastWiFiCheck = millis();
    }
    
    // Handle OTA updates - always handle when WiFi is connected
    if (wifiMgr.isConnected()) {
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
