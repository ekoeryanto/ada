#ifndef DIGITAL_IO_MANAGER_H
#define DIGITAL_IO_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "pins_config.h"

// Digital Input States
enum DigitalInputState {
    DI_LOW = 0,
    DI_HIGH = 1,
    DI_UNKNOWN = 2,
    DI_ERROR = 3
};

// Digital Input Events
enum DigitalInputEvent {
    DI_NO_EVENT = 0,
    DI_RISING_EDGE = 1,
    DI_FALLING_EDGE = 2,
    DI_BOTH_EDGES = 3,
    DI_STATE_CHANGE = 4
};

// Digital Output States
enum DigitalOutputState {
    DO_OFF = 0,
    DO_ON = 1,
    DO_PULSE = 2,
    DO_PWM = 3,
    DO_BLINK = 4,
    DO_ERROR = 5
};

// Digital Input Configuration
struct DigitalInputConfig {
    String name;                    // Input name/description
    String group;                   // Grouping (e.g., "Safety", "Process", "Manual")
    String tags;                    // Tags for filtering/searching
    bool enabled;                   // Enable/disable input
    bool invertLogic;               // Invert logic (HIGH = false, LOW = true)
    unsigned long debounceTime;     // Debounce time in milliseconds
    bool pullupEnabled;             // Enable internal pullup
    
    // Event configuration
    bool edgeDetectionEnabled;      // Enable edge detection
    DigitalInputEvent eventType;    // Type of events to detect
    bool countingEnabled;           // Enable pulse counting
    unsigned long countResetTime;   // Auto-reset counter after time (0 = manual)
    
    // Alarm configuration
    bool alarmEnabled;              // Enable alarm on state change
    DigitalInputState alarmState;   // State that triggers alarm
    unsigned long alarmDelay;       // Delay before alarm triggers
    String alarmMessage;            // Custom alarm message
};

// Digital Output Configuration
struct DigitalOutputConfig {
    String name;                    // Output name/description
    String group;                   // Grouping
    String tags;                    // Tags
    bool enabled;                   // Enable/disable output
    bool invertLogic;               // Invert logic
    
    // Default state
    DigitalOutputState defaultState; // Default state on startup
    bool autoResetEnabled;          // Auto-reset to default after time
    unsigned long autoResetTime;    // Auto-reset time (ms)
    
    // PWM configuration
    int pwmChannel;                 // PWM channel (0-15)
    int pwmFrequency;               // PWM frequency (Hz)
    int pwmResolution;              // PWM resolution (bits)
    
    // Pulse/Blink configuration
    unsigned long pulseWidth;       // Pulse width (ms)
    unsigned long blinkOnTime;      // Blink ON time (ms)
    unsigned long blinkOffTime;     // Blink OFF time (ms)
    int blinkCycles;               // Number of blink cycles (0 = infinite)
    
    // Safety configuration
    bool safetyEnabled;             // Enable safety features
    unsigned long maxOnTime;        // Maximum continuous ON time (safety)
    String interlock;               // Interlock input name
};

// Digital Input Reading
struct DigitalInputReading {
    DigitalInputState currentState;
    DigitalInputState lastState;
    DigitalInputEvent lastEvent;
    unsigned long lastChangeTime;
    unsigned long stateHoldTime;    // Time in current state
    unsigned long pulseCount;       // Pulse counter
    unsigned long totalPulses;      // Total pulses since startup
    bool debounceActive;
    bool alarmActive;
    unsigned long timestamp;
    bool valid;
};

// Digital Output Status
struct DigitalOutputStatus {
    DigitalOutputState currentState;
    DigitalOutputState targetState;
    bool physicalState;             // Actual GPIO state
    int pwmDutyCycle;              // Current PWM duty cycle (0-255 or 0-4095)
    unsigned long stateStartTime;   // When current state started
    unsigned long stateHoldTime;    // Time in current state
    unsigned long operationCount;   // Number of operations
    bool safetyLocked;             // Safety interlock active
    unsigned long lastOperationTime;
    unsigned long timestamp;
    bool valid;
};

// Digital Input Health Data
struct DigitalInputHealthData {
    unsigned long transitionCount;  // Total state transitions
    unsigned long errorCount;       // Error count
    unsigned long lastErrorTime;    // Last error time
    float transitionRate;          // Transitions per minute
    bool stuckDetected;            // Input appears stuck
    bool noiseDetected;            // Excessive transitions detected
    float healthScore;             // Overall health (0-100%)
};

// Digital Output Health Data  
struct DigitalOutputHealthData {
    unsigned long operationCount;   // Total operations
    unsigned long errorCount;       // Error count
    unsigned long lastErrorTime;    // Last error time
    unsigned long totalOnTime;      // Total time spent ON
    float dutyCycleAverage;        // Average duty cycle
    bool overuseDetected;          // Excessive use detected
    float healthScore;             // Overall health (0-100%)
};

class DigitalIOManager {
private:
    // Input configurations and data
    DigitalInputConfig inputConfigs[4];
    DigitalInputReading inputReadings[4];
    DigitalInputHealthData inputHealth[4];
    
    // Output configurations and data
    DigitalOutputConfig outputConfigs[4];
    DigitalOutputStatus outputStatus[4];
    DigitalOutputHealthData outputHealth[4];
    
    // Timing and control
    unsigned long lastUpdateTime;
    unsigned long updateInterval;
    unsigned long lastLogTime;
    unsigned long logInterval;
    bool loggingEnabled;
    
    // Status tracking
    bool initialized;
    unsigned long totalUpdates;
    
    // Real-time streaming
    unsigned long lastStreamTime;
    unsigned long streamInterval;
    bool streamingEnabled;
    
    // Event handling
    void (*inputEventCallback)(int inputIndex, DigitalInputEvent event);
    void (*outputStateCallback)(int outputIndex, DigitalOutputState state);
    
    // Private methods - Input handling
    void updateInputs();
    void processInput(int inputIndex);
    DigitalInputState readInputState(int inputIndex);
    bool checkDebounce(int inputIndex, DigitalInputState newState);
    DigitalInputEvent detectEdge(int inputIndex, DigitalInputState newState);
    void updatePulseCounter(int inputIndex, DigitalInputEvent event);
    void checkInputAlarms(int inputIndex);
    void updateInputHealth(int inputIndex);
    
    // Private methods - Output handling
    void updateOutputs();
    void processOutput(int outputIndex);
    void setPhysicalOutput(int outputIndex, bool state);
    void handlePulseOutput(int outputIndex);
    void handleBlinkOutput(int outputIndex);
    void handlePWMOutput(int outputIndex);
    void checkOutputSafety(int outputIndex);
    void updateOutputHealth(int outputIndex);
    
    // Private methods - Utilities
    int getInputPin(int inputIndex);
    int getOutputPin(int outputIndex);
    void logDigitalData();
    void generateStreamEvent();
    void triggerInputEvent(int inputIndex, DigitalInputEvent event);
    void triggerOutputEvent(int outputIndex, DigitalOutputState state);

public:
    DigitalIOManager();
    
    // Initialization and configuration
    bool begin();
    void setUpdateInterval(unsigned long interval);
    void setLogInterval(unsigned long interval);
    void enableLogging(bool enable = true);
    void enableStreaming(bool enable = true);
    void setStreamInterval(unsigned long interval);
    
    // Input configuration
    void configureInput(int inputIndex, const String& name, const String& group = "", 
                       bool invertLogic = false, unsigned long debounceTime = 50);
    void setInputPullup(int inputIndex, bool enable = true);
    void enableInputCounting(int inputIndex, bool enable = true, unsigned long resetTime = 0);
    void enableInputAlarm(int inputIndex, DigitalInputState alarmState, 
                         unsigned long delay = 0, const String& message = "");
    void setInputEventType(int inputIndex, DigitalInputEvent eventType);
    void enableInput(int inputIndex, bool enable = true);
    
    // Output configuration
    void configureOutput(int outputIndex, const String& name, const String& group = "",
                        bool invertLogic = false, DigitalOutputState defaultState = DO_OFF);
    void configureOutputPWM(int outputIndex, int frequency = 1000, int resolution = 8);
    void configureOutputPulse(int outputIndex, unsigned long pulseWidth = 1000);
    void configureOutputBlink(int outputIndex, unsigned long onTime = 500, 
                             unsigned long offTime = 500, int cycles = 0);
    void configureOutputSafety(int outputIndex, unsigned long maxOnTime = 0, 
                              const String& interlock = "");
    void enableOutput(int outputIndex, bool enable = true);
    
    // Main processing
    void handle();
    bool isInitialized();
    
    // Input operations
    DigitalInputReading getInputReading(int inputIndex);
    DigitalInputState getInputState(int inputIndex);
    bool isInputHigh(int inputIndex);
    bool isInputLow(int inputIndex);
    DigitalInputEvent getLastInputEvent(int inputIndex);
    unsigned long getInputPulseCount(int inputIndex);
    unsigned long getInputStateTime(int inputIndex);
    void resetInputPulseCounter(int inputIndex);
    
    // Output operations
    DigitalOutputStatus getOutputStatus(int outputIndex);
    DigitalOutputState getOutputState(int outputIndex);
    bool isOutputOn(int outputIndex);
    bool isOutputOff(int outputIndex);
    void setOutput(int outputIndex, bool state);
    void setOutputState(int outputIndex, DigitalOutputState state);
    void pulseOutput(int outputIndex, unsigned long duration = 0);
    void blinkOutput(int outputIndex, int cycles = 0);
    void setPWMOutput(int outputIndex, int dutyCycle);
    void toggleOutput(int outputIndex);
    void resetOutput(int outputIndex);
    
    // Batch operations
    void setAllOutputs(bool state);
    void resetAllOutputs();
    void pulseAllOutputs(unsigned long duration = 1000);
    
    // Information and status
    String getInputName(int inputIndex);
    String getOutputName(int outputIndex);
    String getInputInfo(int inputIndex);
    String getOutputInfo(int outputIndex);
    String getSystemInfo();
    
    // Health and diagnostics
    float getInputHealth(int inputIndex);
    float getOutputHealth(int outputIndex);
    bool isInputStuck(int inputIndex);
    bool isInputNoisy(int inputIndex);
    bool isOutputOverused(int outputIndex);
    String getHealthReport();
    void resetHealthData();
    
    // Event callbacks
    void setInputEventCallback(void (*callback)(int, DigitalInputEvent));
    void setOutputStateCallback(void (*callback)(int, DigitalOutputState));
    
    // Alarm and safety
    bool hasInputAlarms();
    bool hasOutputSafetyIssues();
    String getAlarmStatus();
    void acknowledgeAlarms();
    
    // Data access for web interface
    String getInputsJSON();
    String getOutputsJSON();
    String getStatusJSON();
    
    // Advanced features
    void createInputMapping(int inputIndex, int outputIndex, bool direct = true);
    void removeInputMapping(int inputIndex);
    void enableInputOutputMapping(bool enable = true);
    
    // Statistics
    unsigned long getTotalUpdates();
    unsigned long getInputTransitions(int inputIndex);
    unsigned long getOutputOperations(int outputIndex);
    float getInputTransitionRate(int inputIndex);
    float getOutputDutyCycle(int outputIndex);
};

// Global instance
extern DigitalIOManager digitalIOMgr;

#endif // DIGITAL_IO_MANAGER_H