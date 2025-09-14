#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include "config.h"

enum SystemStatus {
    SYSTEM_INITIALIZING,
    SYSTEM_WIFI_CONNECTING,
    SYSTEM_WIFI_CONNECTED,
    SYSTEM_WIFI_FAILED,
    SYSTEM_RUNNING,
    SYSTEM_OTA_UPDATE,
    SYSTEM_ERROR
};

class SystemManager {
private:
    SystemStatus currentStatus;
    Ticker statusLedTicker;
    Ticker heartbeatTicker;
    unsigned long uptimeStart;
    bool debugEnabled;
    
    // Health monitoring
    SystemHealthStatus systemHealth;
    unsigned long lastHealthCheck;
    
    // LED control
    static void toggleStatusLED();
    void setStatusLED(bool state);
    void blinkStatusLED(int interval);
    
    // Status management
    void updateStatusLED();
    static void heartbeatCallback();
    
    // Health monitoring functions
    void updateModuleHealth();
    
public:
    SystemManager();
    
    // Main functions
    bool initialize();
    void loop();
    void setStatus(SystemStatus status);
    
    // Status functions
    SystemStatus getStatus();
    String getStatusString();
    unsigned long getUptime();
    String getUptimeString();
    
    // Health monitoring
    SystemHealthStatus getSystemHealth();
    void setModuleHealth(const String& module, ModuleHealth health);
    bool isSystemHealthy();
    String getHealthReport();
    
    // System functions
    void restart();
    void factoryReset();
    void enableDebug(bool enable);
    
    // Memory functions
    uint32_t getFreeHeap();
    uint8_t getHeapFragmentation();
    uint64_t getChipId();
    
    // Info functions
    String getChipInfo();
    String getSystemInfo();
    void printSystemInfo();
    void printWelcomeBanner();
};

// Global instance
extern SystemManager systemMgr;

#endif // SYSTEM_MANAGER_H
