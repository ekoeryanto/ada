#include "digital_io_manager.h"
#include "sd_manager.h"
#include "ntp_manager.h"
#include "web_server.h"
#include "webhook_handler.h"

// Global instance
DigitalIOManager digitalIOMgr;

DigitalIOManager::DigitalIOManager() :
    lastUpdateTime(0),
    updateInterval(10),     // 10ms for responsive digital I/O
    lastLogTime(0),
    logInterval(60000),     // 1 minute logging interval
    loggingEnabled(true),
    initialized(false),
    totalUpdates(0),
    lastStreamTime(0),
    streamInterval(1000),   // 1 second streaming
    streamingEnabled(true),
    inputEventCallback(nullptr),
    outputStateCallback(nullptr) {
    
    // Initialize input configurations with defaults
    for (int i = 0; i < 4; i++) {
        inputConfigs[i] = {
            "DI" + String(i + 1),           // name
            "General",                       // group
            "digital,input",                 // tags
            true,                           // enabled
            false,                          // invertLogic
            50,                             // debounceTime (ms)
            true,                           // pullupEnabled
            true,                           // edgeDetectionEnabled
            DI_BOTH_EDGES,                  // eventType
            false,                          // countingEnabled
            0,                              // countResetTime
            false,                          // alarmEnabled
            DI_HIGH,                        // alarmState
            0,                              // alarmDelay
            ""                              // alarmMessage
        };
        
        // Initialize input readings
        inputReadings[i] = {
            DI_UNKNOWN,                     // currentState
            DI_UNKNOWN,                     // lastState
            DI_NO_EVENT,                    // lastEvent
            0,                              // lastChangeTime
            0,                              // stateHoldTime
            0,                              // pulseCount
            0,                              // totalPulses
            false,                          // debounceActive
            false,                          // alarmActive
            0,                              // timestamp
            false                           // valid
        };
        
        // Initialize input health
        inputHealth[i] = {
            0,                              // transitionCount
            0,                              // errorCount
            0,                              // lastErrorTime
            0.0,                            // transitionRate
            false,                          // stuckDetected
            false,                          // noiseDetected
            100.0                           // healthScore
        };
    }
    
    // Initialize output configurations with defaults
    for (int i = 0; i < 4; i++) {
        outputConfigs[i] = {
            "DO" + String(i + 1),           // name
            "General",                       // group
            "digital,output",                // tags
            true,                           // enabled
            false,                          // invertLogic
            DO_OFF,                         // defaultState
            false,                          // autoResetEnabled
            0,                              // autoResetTime
            i,                              // pwmChannel
            1000,                           // pwmFrequency
            8,                              // pwmResolution
            1000,                           // pulseWidth
            500,                            // blinkOnTime
            500,                            // blinkOffTime
            0,                              // blinkCycles
            false,                          // safetyEnabled
            0,                              // maxOnTime
            ""                              // interlock
        };
        
        // Initialize output status
        outputStatus[i] = {
            DO_OFF,                         // currentState
            DO_OFF,                         // targetState
            false,                          // physicalState
            0,                              // pwmDutyCycle
            0,                              // stateStartTime
            0,                              // stateHoldTime
            0,                              // operationCount
            false,                          // safetyLocked
            0,                              // lastOperationTime
            0,                              // timestamp
            false                           // valid
        };
        
        // Initialize output health
        outputHealth[i] = {
            0,                              // operationCount
            0,                              // errorCount
            0,                              // lastErrorTime
            0,                              // totalOnTime
            0.0,                            // dutyCycleAverage
            false,                          // overuseDetected
            100.0                           // healthScore
        };
    }
}

bool DigitalIOManager::begin() {
    if (DEBUG_ENABLED) {
        Serial.println("[DIO] Initializing Digital IO Manager...");
    }
    
    // Configure input pins
    for (int i = 0; i < 4; i++) {
        if (inputConfigs[i].enabled) {
            int pin = getInputPin(i);
            if (inputConfigs[i].pullupEnabled) {
                pinMode(pin, INPUT_PULLUP);
            } else {
                pinMode(pin, INPUT);
            }
            
            if (DEBUG_ENABLED) {
                Serial.printf("[DIO] Configured input %d (%s) on pin %d\n", 
                             i, inputConfigs[i].name.c_str(), pin);
            }
        }
    }
    
    // Configure output pins and PWM
    for (int i = 0; i < 4; i++) {
        if (outputConfigs[i].enabled) {
            int pin = getOutputPin(i);
            pinMode(pin, OUTPUT);
            
            // Setup PWM if configured
            if (outputConfigs[i].defaultState == DO_PWM) {
                ledcSetup(outputConfigs[i].pwmChannel, 
                         outputConfigs[i].pwmFrequency, 
                         outputConfigs[i].pwmResolution);
                ledcAttachPin(pin, outputConfigs[i].pwmChannel);
            }
            
            // Set default state
            setOutputState(i, outputConfigs[i].defaultState);
            
            if (DEBUG_ENABLED) {
                Serial.printf("[DIO] Configured output %d (%s) on pin %d\n", 
                             i, outputConfigs[i].name.c_str(), pin);
            }
        }
    }
    
    initialized = true;
    lastUpdateTime = millis();
    
    if (DEBUG_ENABLED) {
        Serial.println("[DIO] Digital IO Manager initialized successfully");
        Serial.printf("[DIO] Update interval: %lums, Log interval: %lums\n", 
                     updateInterval, logInterval);
    }
    
    return true;
}

void DigitalIOManager::handle() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // High-frequency update for digital I/O
    if (currentTime - lastUpdateTime >= updateInterval) {
        updateInputs();
        updateOutputs();
        lastUpdateTime = currentTime;
        totalUpdates++;
    }
    
    // Periodic data logging
    if (loggingEnabled && (currentTime - lastLogTime >= logInterval)) {
        logDigitalData();
        lastLogTime = currentTime;
    }
    
    // Real-time streaming
    if (streamingEnabled && (currentTime - lastStreamTime >= streamInterval)) {
        generateStreamEvent();
        lastStreamTime = currentTime;
    }
}

void DigitalIOManager::updateInputs() {
    for (int i = 0; i < 4; i++) {
        if (inputConfigs[i].enabled) {
            processInput(i);
        }
    }
}

void DigitalIOManager::processInput(int inputIndex) {
    if (inputIndex < 0 || inputIndex >= 4) return;
    
    unsigned long currentTime = millis();
    DigitalInputState newState = readInputState(inputIndex);
    DigitalInputReading& reading = inputReadings[inputIndex];
    
    // Check debounce
    if (checkDebounce(inputIndex, newState)) {
        // State confirmed after debounce
        if (newState != reading.currentState) {
            // State change detected
            reading.lastState = reading.currentState;
            reading.currentState = newState;
            reading.lastChangeTime = currentTime;
            reading.stateHoldTime = 0;
            
            // Detect edge event
            DigitalInputEvent event = detectEdge(inputIndex, newState);
            reading.lastEvent = event;
            
            // Update pulse counter
            updatePulseCounter(inputIndex, event);
            
            // Update health tracking
            updateInputHealth(inputIndex);
            
            // Trigger event callback
            if (inputEventCallback && event != DI_NO_EVENT) {
                triggerInputEvent(inputIndex, event);
            }
            
            // Check alarms
            checkInputAlarms(inputIndex);
            
            if (DEBUG_ENABLED && totalUpdates % 100 == 0) {
                Serial.printf("[DIO] Input %d (%s): %s -> %s\n", 
                             inputIndex, inputConfigs[inputIndex].name.c_str(),
                             reading.lastState == DI_HIGH ? "HIGH" : "LOW",
                             reading.currentState == DI_HIGH ? "HIGH" : "LOW");
            }
        } else {
            // Update state hold time
            reading.stateHoldTime = currentTime - reading.lastChangeTime;
        }
    }
    
    reading.timestamp = currentTime;
    reading.valid = true;
}

DigitalInputState DigitalIOManager::readInputState(int inputIndex) {
    int pin = getInputPin(inputIndex);
    bool pinState = digitalRead(pin);
    
    // Apply logic inversion if configured
    if (inputConfigs[inputIndex].invertLogic) {
        pinState = !pinState;
    }
    
    return pinState ? DI_HIGH : DI_LOW;
}

bool DigitalIOManager::checkDebounce(int inputIndex, DigitalInputState newState) {
    static DigitalInputState pendingState[4] = {DI_UNKNOWN, DI_UNKNOWN, DI_UNKNOWN, DI_UNKNOWN};
    static unsigned long debounceStartTime[4] = {0, 0, 0, 0};
    
    DigitalInputReading& reading = inputReadings[inputIndex];
    unsigned long currentTime = millis();
    
    if (newState != reading.currentState) {
        if (pendingState[inputIndex] != newState) {
            // New state change - start debounce timer
            pendingState[inputIndex] = newState;
            debounceStartTime[inputIndex] = currentTime;
            reading.debounceActive = true;
            return false;
        } else {
            // Same pending state - check if debounce time elapsed
            if (currentTime - debounceStartTime[inputIndex] >= inputConfigs[inputIndex].debounceTime) {
                reading.debounceActive = false;
                return true;  // State confirmed
            }
            return false;  // Still debouncing
        }
    } else {
        // State hasn't changed
        pendingState[inputIndex] = DI_UNKNOWN;
        reading.debounceActive = false;
        return false;
    }
}

DigitalInputEvent DigitalIOManager::detectEdge(int inputIndex, DigitalInputState newState) {
    DigitalInputReading& reading = inputReadings[inputIndex];
    DigitalInputEvent event = DI_NO_EVENT;
    
    if (reading.lastState == DI_LOW && newState == DI_HIGH) {
        event = DI_RISING_EDGE;
    } else if (reading.lastState == DI_HIGH && newState == DI_LOW) {
        event = DI_FALLING_EDGE;
    }
    
    // Check if this event type is enabled
    DigitalInputEvent configuredEvent = inputConfigs[inputIndex].eventType;
    if (configuredEvent == DI_BOTH_EDGES || configuredEvent == event) {
        return event;
    }
    
    return DI_NO_EVENT;
}

void DigitalIOManager::updatePulseCounter(int inputIndex, DigitalInputEvent event) {
    if (!inputConfigs[inputIndex].countingEnabled) return;
    
    DigitalInputReading& reading = inputReadings[inputIndex];
    
    // Count on rising edge by default
    if (event == DI_RISING_EDGE) {
        reading.pulseCount++;
        reading.totalPulses++;
    }
    
    // Auto-reset counter if configured
    if (inputConfigs[inputIndex].countResetTime > 0) {
        unsigned long timeSinceLastPulse = millis() - reading.lastChangeTime;
        if (timeSinceLastPulse > inputConfigs[inputIndex].countResetTime) {
            reading.pulseCount = 0;
        }
    }
}

void DigitalIOManager::updateOutputs() {
    for (int i = 0; i < 4; i++) {
        if (outputConfigs[i].enabled) {
            processOutput(i);
        }
    }
}

void DigitalIOManager::processOutput(int outputIndex) {
    if (outputIndex < 0 || outputIndex >= 4) return;
    
    DigitalOutputStatus& status = outputStatus[outputIndex];
    unsigned long currentTime = millis();
    
    // Update state hold time
    status.stateHoldTime = currentTime - status.stateStartTime;
    
    // Handle different output states
    switch (status.currentState) {
        case DO_PULSE:
            handlePulseOutput(outputIndex);
            break;
            
        case DO_BLINK:
            handleBlinkOutput(outputIndex);
            break;
            
        case DO_PWM:
            handlePWMOutput(outputIndex);
            break;
            
        case DO_ON:
        case DO_OFF:
            // Static states - check safety and auto-reset
            checkOutputSafety(outputIndex);
            break;
            
        default:
            break;
    }
    
    // Check auto-reset
    if (outputConfigs[outputIndex].autoResetEnabled && 
        status.currentState != outputConfigs[outputIndex].defaultState &&
        status.stateHoldTime >= outputConfigs[outputIndex].autoResetTime) {
        
        setOutputState(outputIndex, outputConfigs[outputIndex].defaultState);
    }
    
    // Update health tracking
    updateOutputHealth(outputIndex);
    
    status.timestamp = currentTime;
    status.valid = true;
}

void DigitalIOManager::handlePulseOutput(int outputIndex) {
    DigitalOutputStatus& status = outputStatus[outputIndex];
    unsigned long pulseWidth = outputConfigs[outputIndex].pulseWidth;
    
    if (status.stateHoldTime >= pulseWidth) {
        // Pulse completed - return to OFF
        setOutputState(outputIndex, DO_OFF);
    }
}

void DigitalIOManager::handleBlinkOutput(int outputIndex) {
    DigitalOutputStatus& status = outputStatus[outputIndex];
    DigitalOutputConfig& config = outputConfigs[outputIndex];
    static int blinkCycleCount[4] = {0, 0, 0, 0};
    static bool blinkPhase[4] = {false, false, false, false};  // false = OFF, true = ON
    static unsigned long lastBlinkTime[4] = {0, 0, 0, 0};
    
    unsigned long currentTime = millis();
    unsigned long phaseTime = blinkPhase[outputIndex] ? config.blinkOnTime : config.blinkOffTime;
    
    if (currentTime - lastBlinkTime[outputIndex] >= phaseTime) {
        // Switch blink phase
        blinkPhase[outputIndex] = !blinkPhase[outputIndex];
        setPhysicalOutput(outputIndex, blinkPhase[outputIndex]);
        lastBlinkTime[outputIndex] = currentTime;
        
        // Count cycles (complete cycle = ON -> OFF)
        if (!blinkPhase[outputIndex]) {
            blinkCycleCount[outputIndex]++;
            
            // Check if blink cycles completed
            if (config.blinkCycles > 0 && blinkCycleCount[outputIndex] >= config.blinkCycles) {
                blinkCycleCount[outputIndex] = 0;
                setOutputState(outputIndex, DO_OFF);
            }
        }
    }
}

void DigitalIOManager::handlePWMOutput(int outputIndex) {
    // PWM is handled by hardware - just monitor
    DigitalOutputStatus& status = outputStatus[outputIndex];
    status.physicalState = (status.pwmDutyCycle > 0);
}

void DigitalIOManager::setPhysicalOutput(int outputIndex, bool state) {
    int pin = getOutputPin(outputIndex);
    DigitalOutputStatus& status = outputStatus[outputIndex];
    
    // Apply logic inversion if configured
    bool actualState = outputConfigs[outputIndex].invertLogic ? !state : state;
    
    digitalWrite(pin, actualState);
    status.physicalState = state;  // Store logical state
}

int DigitalIOManager::getInputPin(int inputIndex) {
    switch (inputIndex) {
        case 0: return DI1_PIN;
        case 1: return DI2_PIN;
        case 2: return DI3_PIN;
        case 3: return DI4_PIN;
        default: return -1;
    }
}

int DigitalIOManager::getOutputPin(int outputIndex) {
    switch (outputIndex) {
        case 0: return DO1_PIN;
        case 1: return DO2_PIN;
        case 2: return DO3_PIN;
        case 3: return DO4_PIN;
        default: return -1;
    }
}

// Public configuration methods
void DigitalIOManager::configureInput(int inputIndex, const String& name, const String& group,
                                     bool invertLogic, unsigned long debounceTime) {
    if (inputIndex < 0 || inputIndex >= 4) return;
    
    DigitalInputConfig& config = inputConfigs[inputIndex];
    config.name = name;
    config.group = group;
    config.invertLogic = invertLogic;
    config.debounceTime = debounceTime;
    
    if (DEBUG_ENABLED) {
        Serial.printf("[DIO] Input %d configured: %s, debounce: %lums\n", 
                     inputIndex, name.c_str(), debounceTime);
    }
}

void DigitalIOManager::configureOutput(int outputIndex, const String& name, const String& group,
                                      bool invertLogic, DigitalOutputState defaultState) {
    if (outputIndex < 0 || outputIndex >= 4) return;
    
    DigitalOutputConfig& config = outputConfigs[outputIndex];
    config.name = name;
    config.group = group;
    config.invertLogic = invertLogic;
    config.defaultState = defaultState;
    
    if (DEBUG_ENABLED) {
        Serial.printf("[DIO] Output %d configured: %s, default: %d\n", 
                     outputIndex, name.c_str(), (int)defaultState);
    }
}

// Public input operations
DigitalInputReading DigitalIOManager::getInputReading(int inputIndex) {
    if (inputIndex < 0 || inputIndex >= 4) {
        return {DI_ERROR, DI_ERROR, DI_NO_EVENT, 0, 0, 0, 0, false, false, 0, false};
    }
    return inputReadings[inputIndex];
}

DigitalInputState DigitalIOManager::getInputState(int inputIndex) {
    if (inputIndex < 0 || inputIndex >= 4) return DI_ERROR;
    return inputReadings[inputIndex].currentState;
}

bool DigitalIOManager::isInputHigh(int inputIndex) {
    return getInputState(inputIndex) == DI_HIGH;
}

bool DigitalIOManager::isInputLow(int inputIndex) {
    return getInputState(inputIndex) == DI_LOW;
}

// Public output operations
void DigitalIOManager::setOutput(int outputIndex, bool state) {
    setOutputState(outputIndex, state ? DO_ON : DO_OFF);
}

void DigitalIOManager::setOutputState(int outputIndex, DigitalOutputState state) {
    if (outputIndex < 0 || outputIndex >= 4) return;
    if (!outputConfigs[outputIndex].enabled) return;
    
    DigitalOutputStatus& status = outputStatus[outputIndex];
    
    // Check safety interlock
    if (outputConfigs[outputIndex].safetyEnabled && status.safetyLocked) {
        if (DEBUG_ENABLED) {
            Serial.printf("[DIO] Output %d safety locked - operation denied\n", outputIndex);
        }
        return;
    }
    
    // Update status
    status.currentState = state;
    status.targetState = state;
    status.stateStartTime = millis();
    status.operationCount++;
    status.lastOperationTime = millis();
    
    // Handle different states
    switch (state) {
        case DO_OFF:
            setPhysicalOutput(outputIndex, false);
            status.pwmDutyCycle = 0;
            break;
            
        case DO_ON:
            setPhysicalOutput(outputIndex, true);
            status.pwmDutyCycle = 255;  // Full duty cycle
            break;
            
        case DO_PULSE:
            setPhysicalOutput(outputIndex, true);
            // Will be handled in processOutput()
            break;
            
        case DO_PWM:
            // PWM handled separately via setPWMOutput()
            break;
            
        case DO_BLINK:
            // Blink handled in processOutput()
            break;
            
        default:
            break;
    }
    
    // Trigger callback
    if (outputStateCallback) {
        triggerOutputEvent(outputIndex, state);
    }
    
    if (DEBUG_ENABLED) {
        Serial.printf("[DIO] Output %d (%s) set to state %d\n", 
                     outputIndex, outputConfigs[outputIndex].name.c_str(), (int)state);
    }
}

void DigitalIOManager::setPWMOutput(int outputIndex, int dutyCycle) {
    if (outputIndex < 0 || outputIndex >= 4) return;
    
    DigitalOutputStatus& status = outputStatus[outputIndex];
    DigitalOutputConfig& config = outputConfigs[outputIndex];
    
    // Clamp duty cycle to valid range
    int maxDutyCycle = (1 << config.pwmResolution) - 1;
    dutyCycle = constrain(dutyCycle, 0, maxDutyCycle);
    
    // Set PWM
    ledcWrite(config.pwmChannel, dutyCycle);
    
    // Update status
    status.currentState = DO_PWM;
    status.pwmDutyCycle = dutyCycle;
    status.physicalState = (dutyCycle > 0);
    
    if (DEBUG_ENABLED) {
        Serial.printf("[DIO] Output %d PWM set to %d/%d\n", 
                     outputIndex, dutyCycle, maxDutyCycle);
    }
}

void DigitalIOManager::pulseOutput(int outputIndex, unsigned long duration) {
    if (duration > 0) {
        outputConfigs[outputIndex].pulseWidth = duration;
    }
    setOutputState(outputIndex, DO_PULSE);
}

void DigitalIOManager::toggleOutput(int outputIndex) {
    DigitalOutputState currentState = getOutputState(outputIndex);
    if (currentState == DO_ON) {
        setOutput(outputIndex, false);
    } else {
        setOutput(outputIndex, true);
    }
}

// Information methods
String DigitalIOManager::getInputName(int inputIndex) {
    if (inputIndex < 0 || inputIndex >= 4) return "Invalid";
    return inputConfigs[inputIndex].name;
}

String DigitalIOManager::getOutputName(int outputIndex) {
    if (outputIndex < 0 || outputIndex >= 4) return "Invalid";
    return outputConfigs[outputIndex].name;
}

String DigitalIOManager::getSystemInfo() {
    String info = "Digital IO Manager Status:\n";
    info += "Initialized: " + String(initialized ? "Yes" : "No") + "\n";
    info += "Total Updates: " + String(totalUpdates) + "\n";
    info += "Update Interval: " + String(updateInterval) + "ms\n\n";
    
    // Input summary
    info += "Digital Inputs:\n";
    for (int i = 0; i < 4; i++) {
        info += "  " + inputConfigs[i].name + ": ";
        info += (inputReadings[i].currentState == DI_HIGH ? "HIGH" : "LOW");
        info += " (Pulses: " + String(inputReadings[i].totalPulses) + ")\n";
    }
    
    info += "\nDigital Outputs:\n";
    for (int i = 0; i < 4; i++) {
        info += "  " + outputConfigs[i].name + ": ";
        info += (outputStatus[i].physicalState ? "ON" : "OFF");
        info += " (Ops: " + String(outputStatus[i].operationCount) + ")\n";
    }
    
    return info;
}

// Health monitoring and diagnostics
void DigitalIOManager::updateInputHealth(int inputIndex) {
    DigitalInputHealthData& health = inputHealth[inputIndex];
    health.transitionCount++;
    
    // Calculate transition rate (transitions per minute)
    unsigned long currentTime = millis();
    static unsigned long lastRateCalc[4] = {0, 0, 0, 0};
    static unsigned long rateCounter[4] = {0, 0, 0, 0};
    
    rateCounter[inputIndex]++;
    
    if (currentTime - lastRateCalc[inputIndex] >= 60000) {  // 1 minute
        health.transitionRate = rateCounter[inputIndex];
        rateCounter[inputIndex] = 0;
        lastRateCalc[inputIndex] = currentTime;
        
        // Detect noise (too many transitions)
        health.noiseDetected = (health.transitionRate > 100);  // >100 transitions/min
        
        // Calculate health score
        float score = 100.0;
        if (health.noiseDetected) score -= 30.0;
        if (health.stuckDetected) score -= 40.0;
        if (health.errorCount > 0) score -= (health.errorCount * 5.0);
        
        health.healthScore = max(0.0f, min(100.0f, score));
    }
}

void DigitalIOManager::updateOutputHealth(int outputIndex) {
    DigitalOutputHealthData& health = outputHealth[outputIndex];
    DigitalOutputStatus& status = outputStatus[outputIndex];
    
    health.operationCount = status.operationCount;
    
    // Update total ON time
    if (status.physicalState) {
        health.totalOnTime += updateInterval;
    }
    
    // Calculate average duty cycle
    unsigned long totalTime = millis();
    if (totalTime > 0) {
        health.dutyCycleAverage = (float)health.totalOnTime / totalTime * 100.0;
    }
    
    // Detect overuse
    health.overuseDetected = (health.dutyCycleAverage > 80.0);  // >80% duty cycle
    
    // Calculate health score
    float score = 100.0;
    if (health.overuseDetected) score -= 25.0;
    if (health.errorCount > 0) score -= (health.errorCount * 5.0);
    
    health.healthScore = max(0.0f, min(100.0f, score));
}

void DigitalIOManager::checkInputAlarms(int inputIndex) {
    if (!inputConfigs[inputIndex].alarmEnabled) return;
    
    DigitalInputReading& reading = inputReadings[inputIndex];
    DigitalInputConfig& config = inputConfigs[inputIndex];
    
    if (reading.currentState == config.alarmState) {
        if (!reading.alarmActive && reading.stateHoldTime >= config.alarmDelay) {
            reading.alarmActive = true;
            
            // Trigger alarm notification
            String message = config.alarmMessage.isEmpty() ? 
                           ("Input " + config.name + " alarm triggered") : 
                           config.alarmMessage;
            
            if (DEBUG_ENABLED) {
                Serial.println("[DIO] ALARM: " + message);
            }
            
            // Send webhook notification
            webhookHandler.sendAlarmTriggered("digital_input_" + String(inputIndex), 
                                            "INPUT_ALARM", 
                                            reading.currentState == DI_HIGH ? 1.0 : 0.0, 
                                            config.alarmState == DI_HIGH ? 1.0 : 0.0);
        }
    } else {
        reading.alarmActive = false;
    }
}

void DigitalIOManager::checkOutputSafety(int outputIndex) {
    if (!outputConfigs[outputIndex].safetyEnabled) return;
    
    DigitalOutputStatus& status = outputStatus[outputIndex];
    DigitalOutputConfig& config = outputConfigs[outputIndex];
    
    // Check maximum ON time
    if (config.maxOnTime > 0 && status.physicalState && 
        status.stateHoldTime >= config.maxOnTime) {
        
        status.safetyLocked = true;
        setOutputState(outputIndex, DO_OFF);
        
        if (DEBUG_ENABLED) {
            Serial.printf("[DIO] SAFETY: Output %d exceeded max ON time - locked\n", outputIndex);
        }
    }
    
    // Check interlock input
    if (!config.interlock.isEmpty()) {
        // Find interlock input by name
        for (int i = 0; i < 4; i++) {
            if (inputConfigs[i].name == config.interlock) {
                if (inputReadings[i].currentState == DI_LOW) {
                    status.safetyLocked = true;
                    setOutputState(outputIndex, DO_OFF);
                    
                    if (DEBUG_ENABLED) {
                        Serial.printf("[DIO] SAFETY: Output %d interlocked by %s\n", 
                                     outputIndex, config.interlock.c_str());
                    }
                }
                break;
            }
        }
    }
}

void DigitalIOManager::logDigitalData() {
    if (!initialized || !loggingEnabled || !sdMgr.isMounted()) return;
    
    String logEntry = "";
    
    // Add input states
    for (int i = 0; i < 4; i++) {
        if (i > 0) logEntry += ",";
        logEntry += String(inputReadings[i].currentState) + "," +
                   String(inputReadings[i].pulseCount) + "," +
                   String(inputReadings[i].stateHoldTime);
    }
    
    // Add output states
    for (int i = 0; i < 4; i++) {
        logEntry += ",";
        logEntry += String(outputStatus[i].currentState) + "," +
                   String(outputStatus[i].physicalState ? 1 : 0) + "," +
                   String(outputStatus[i].operationCount);
    }
    
    // Log to SD card
    if (sdMgr.logDataWithTimestamp("DIO," + logEntry)) {
        if (DEBUG_ENABLED) {
            Serial.println("[DIO] Data logged: " + logEntry);
        }
    }
}

void DigitalIOManager::generateStreamEvent() {
    if (!streamingEnabled) return;
    
    // Generate JSON for WebSocket streaming
    String streamData = "{";
    streamData += "\"type\":\"digital_io\",";
    streamData += "\"timestamp\":" + String(millis()) + ",";
    
    // Input data
    streamData += "\"inputs\":[";
    for (int i = 0; i < 4; i++) {
        if (i > 0) streamData += ",";
        streamData += "{";
        streamData += "\"name\":\"" + inputConfigs[i].name + "\",";
        streamData += "\"state\":" + String(inputReadings[i].currentState) + ",";
        streamData += "\"pulses\":" + String(inputReadings[i].pulseCount) + ",";
        streamData += "\"health\":" + String(inputHealth[i].healthScore, 1);
        streamData += "}";
    }
    streamData += "],";
    
    // Output data
    streamData += "\"outputs\":[";
    for (int i = 0; i < 4; i++) {
        if (i > 0) streamData += ",";
        streamData += "{";
        streamData += "\"name\":\"" + outputConfigs[i].name + "\",";
        streamData += "\"state\":" + String(outputStatus[i].currentState) + ",";
        streamData += "\"physical\":" + String(outputStatus[i].physicalState ? 1 : 0) + ",";
        streamData += "\"health\":" + String(outputHealth[i].healthScore, 1);
        streamData += "}";
    }
    streamData += "]";
    streamData += "}";
    
    // Broadcast to WebSocket clients
    extern WebServerHandler webServer;
    webServer.broadcastToWebSocket(streamData);
}

// Placeholder implementations for remaining methods
void DigitalIOManager::setUpdateInterval(unsigned long interval) { updateInterval = max(interval, 1UL); }
void DigitalIOManager::setLogInterval(unsigned long interval) { logInterval = max(interval, 1000UL); }
void DigitalIOManager::enableLogging(bool enable) { loggingEnabled = enable; }
void DigitalIOManager::enableStreaming(bool enable) { streamingEnabled = enable; }
void DigitalIOManager::setStreamInterval(unsigned long interval) { streamInterval = max(interval, 100UL); }
bool DigitalIOManager::isInitialized() { return initialized; }

// Additional helper methods
void DigitalIOManager::triggerInputEvent(int inputIndex, DigitalInputEvent event) {
    if (inputEventCallback) {
        inputEventCallback(inputIndex, event);
    }
}

void DigitalIOManager::triggerOutputEvent(int outputIndex, DigitalOutputState state) {
    if (outputStateCallback) {
        outputStateCallback(outputIndex, state);
    }
}

// Getters for remaining methods
DigitalOutputStatus DigitalIOManager::getOutputStatus(int outputIndex) {
    if (outputIndex < 0 || outputIndex >= 4) {
        return {DO_ERROR, DO_ERROR, false, 0, 0, 0, 0, false, 0, 0, false};
    }
    return outputStatus[outputIndex];
}

DigitalOutputState DigitalIOManager::getOutputState(int outputIndex) {
    if (outputIndex < 0 || outputIndex >= 4) return DO_ERROR;
    return outputStatus[outputIndex].currentState;
}

bool DigitalIOManager::isOutputOn(int outputIndex) {
    return outputStatus[outputIndex].physicalState;
}

bool DigitalIOManager::isOutputOff(int outputIndex) {
    return !outputStatus[outputIndex].physicalState;
}

unsigned long DigitalIOManager::getInputPulseCount(int inputIndex) {
    if (inputIndex < 0 || inputIndex >= 4) return 0;
    return inputReadings[inputIndex].pulseCount;
}

unsigned long DigitalIOManager::getTotalUpdates() { return totalUpdates; }

// Stub implementations for advanced features (would be fully implemented)
void DigitalIOManager::setInputPullup(int inputIndex, bool enable) {/* Implementation */}
void DigitalIOManager::enableInputCounting(int inputIndex, bool enable, unsigned long resetTime) {
    if (inputIndex >= 0 && inputIndex < 4) {
        inputConfigs[inputIndex].countingEnabled = enable;
        inputConfigs[inputIndex].countResetTime = resetTime;
    }
}
void DigitalIOManager::enableInputAlarm(int inputIndex, DigitalInputState alarmState, unsigned long delay, const String& message) {
    if (inputIndex >= 0 && inputIndex < 4) {
        inputConfigs[inputIndex].alarmEnabled = true;
        inputConfigs[inputIndex].alarmState = alarmState;
        inputConfigs[inputIndex].alarmDelay = delay;
        inputConfigs[inputIndex].alarmMessage = message;
    }
}
void DigitalIOManager::resetInputPulseCounter(int inputIndex) { if (inputIndex >= 0 && inputIndex < 4) inputReadings[inputIndex].pulseCount = 0; }
void DigitalIOManager::setAllOutputs(bool state) { for (int i = 0; i < 4; i++) setOutput(i, state); }
void DigitalIOManager::resetAllOutputs() { for (int i = 0; i < 4; i++) setOutput(i, false); }
String DigitalIOManager::getInputInfo(int inputIndex) { return "Input info placeholder"; }
String DigitalIOManager::getOutputInfo(int outputIndex) { return "Output info placeholder"; }
float DigitalIOManager::getInputHealth(int inputIndex) { return (inputIndex >= 0 && inputIndex < 4) ? inputHealth[inputIndex].healthScore : 0.0; }
float DigitalIOManager::getOutputHealth(int outputIndex) { return (outputIndex >= 0 && outputIndex < 4) ? outputHealth[outputIndex].healthScore : 0.0; }
String DigitalIOManager::getHealthReport() { return "Health report placeholder"; }
void DigitalIOManager::resetHealthData() {/* Implementation */}
String DigitalIOManager::getInputsJSON() { return "{}"; }
String DigitalIOManager::getOutputsJSON() { return "{}"; }
String DigitalIOManager::getStatusJSON() { return "{}"; }

// Callback setters
void DigitalIOManager::setInputEventCallback(void (*callback)(int, DigitalInputEvent)) {
    inputEventCallback = callback;
}

void DigitalIOManager::setOutputStateCallback(void (*callback)(int, DigitalOutputState)) {
    outputStateCallback = callback;
}