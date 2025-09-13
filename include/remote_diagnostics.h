#ifndef REMOTE_DIAGNOSTICS_H
#define REMOTE_DIAGNOSTICS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// System performance metrics
struct SystemMetrics {
    unsigned long uptime;
    unsigned long freeHeap;
    unsigned long totalHeap;
    unsigned long minFreeHeap;
    float cpuUsage;             // Estimated CPU usage %
    float memoryUsage;          // Memory usage %
    unsigned long resetReason;
    float chipTemperature;      // ESP32 internal temperature
    unsigned long wifiReconnects;
    unsigned long lastResetTime;
};

// Network diagnostics
struct NetworkDiagnostics {
    bool wifiConnected;
    String ssid;
    int rssi;
    String ip;
    String gateway;
    String dns;
    unsigned long connectionTime;
    unsigned long disconnectionCount;
    unsigned long packetsSent;
    unsigned long packetsReceived;
    unsigned long packetsLost;
    float packetLossRate;
    int pingLatency;            // ms
    String macAddress;
};

// Storage diagnostics
struct StorageDiagnostics {
    bool sdCardMounted;
    unsigned long totalSpace;
    unsigned long usedSpace;
    unsigned long freeSpace;
    float usagePercentage;
    unsigned long filesCount;
    unsigned long lastWriteTime;
    unsigned long writeErrors;
    unsigned long readErrors;
    String cardType;
    float cardSpeed;            // MB/s
};

// Sensor diagnostics
struct SensorDiagnostics {
    int sensorId;
    bool isOnline;
    bool isResponding;
    unsigned long lastReadTime;
    unsigned long successfulReads;
    unsigned long failedReads;
    float errorRate;
    float avgResponseTime;
    float signalQuality;        // 0-100%
    String lastError;
    unsigned long calibrationDate;
    bool needsCalibration;
    float driftRate;            // Units per day
};

// Remote command structure
struct RemoteCommand {
    String command;
    String parameters;
    unsigned long timestamp;
    String requestId;
    bool executed;
    String result;
    String error;
};

// Diagnostic alert levels
enum DiagnosticLevel {
    DIAG_INFO = 0,
    DIAG_WARNING = 1,
    DIAG_ERROR = 2,
    DIAG_CRITICAL = 3
};

// Diagnostic alert structure
struct DiagnosticAlert {
    DiagnosticLevel level;
    String category;            // "system", "network", "storage", "sensor"
    String message;
    String recommendation;
    unsigned long timestamp;
    bool acknowledged;
    String source;
};

class RemoteDiagnostics {
private:
    // System metrics
    SystemMetrics systemMetrics;
    NetworkDiagnostics networkDiag;
    StorageDiagnostics storageDiag;
    SensorDiagnostics sensorDiag[3];  // For 3 analog sensors
    
    // Diagnostic alerts
    static const int MAX_ALERTS = 20;
    DiagnosticAlert alerts[MAX_ALERTS];
    int alertCount;
    int alertIndex;
    
    // Remote commands queue
    static const int MAX_COMMANDS = 10;
    RemoteCommand commandQueue[MAX_COMMANDS];
    int commandCount;
    int commandIndex;
    
    // Configuration
    bool diagnosticsEnabled;
    unsigned long diagnosticInterval;
    unsigned long lastDiagnosticTime;
    unsigned long lastSystemCheck;
    unsigned long lastNetworkCheck;
    unsigned long lastStorageCheck;
    bool remoteCommandsEnabled;
    bool initialized;
    
    // Performance monitoring
    unsigned long loopCounter;
    unsigned long lastLoopTime;
    unsigned long maxLoopTime;
    float avgLoopTime;
    
    // Static constants
    static const unsigned long DEFAULT_DIAGNOSTIC_INTERVAL = 60000;    // 1 minute
    static const unsigned long SYSTEM_CHECK_INTERVAL = 30000;          // 30 seconds
    static const unsigned long NETWORK_CHECK_INTERVAL = 45000;         // 45 seconds
    static const unsigned long STORAGE_CHECK_INTERVAL = 120000;        // 2 minutes
    
    // Private methods
    void updateSystemMetrics();
    void updateNetworkDiagnostics();
    void updateStorageDiagnostics();
    void updateSensorDiagnostics();
    void checkSystemHealth();
    void checkNetworkHealth();
    void checkStorageHealth();
    void checkSensorHealth(int sensorIndex);
    void addAlert(DiagnosticLevel level, const String& category, const String& message, const String& recommendation = "");
    void processRemoteCommands();
    void executeCommand(RemoteCommand& cmd);
    void logDiagnostics();
    float calculateCPUUsage();
    int pingTest(const String& host = "8.8.8.8");
    void performSelfTest();
    
public:
    RemoteDiagnostics();
    
    // Initialization
    bool begin();
    void setDiagnosticInterval(unsigned long interval);
    void enableRemoteCommands(bool enable);
    
    // Main processing
    void handle();
    void runDiagnostics();
    
    // Metrics access
    SystemMetrics getSystemMetrics();
    NetworkDiagnostics getNetworkDiagnostics();
    StorageDiagnostics getStorageDiagnostics();
    SensorDiagnostics getSensorDiagnostics(int sensorIndex);
    
    // JSON exports
    String getSystemMetricsJSON();
    String getNetworkDiagnosticsJSON();
    String getStorageDiagnosticsJSON();
    String getSensorDiagnosticsJSON(int sensorIndex);
    String getAllDiagnosticsJSON();
    String getDiagnosticsSummary();
    
    // Alerts management
    void clearAlerts();
    void acknowledgeAlert(int alertIndex);
    String getAlertsJSON();
    int getAlertCount();
    DiagnosticAlert getAlert(int index);
    
    // Remote commands
    void addRemoteCommand(const String& command, const String& parameters, const String& requestId = "");
    String getCommandResultsJSON();
    void clearCommandHistory();
    bool hasCommandResults();
    
    // Health checks
    String performHealthCheck();
    String generateDiagnosticReport();
    bool isSystemHealthy();
    float getOverallHealthScore();
    
    // Performance monitoring
    void recordLoopTime(unsigned long loopTime);
    String getPerformanceMetrics();
    
    // Remote troubleshooting
    String runNetworkTest();
    String runStorageTest();
    String runSensorTest(int sensorIndex = -1);  // -1 for all sensors
    String runMemoryTest();
    String getSystemInfo();
    
    // Maintenance functions
    void restartSystem(const String& reason = "Remote restart");
    void resetNetworkSettings();
    void formatSDCard();
    void calibrateAllSensors();
    void factoryReset();
    
    // Status
    bool isInitialized() const;
    String getStatus();
    unsigned long getLastDiagnosticTime();
};

// Global instance
extern RemoteDiagnostics remoteDiag;

#endif // REMOTE_DIAGNOSTICS_H