#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include "RTClib.h"
#include "config.h"
#include "pins_config.h"

enum NTPStatus {
    NTP_NOT_INITIALIZED,
    NTP_INITIALIZING,
    NTP_SYNCING,
    NTP_SYNCED,
    NTP_SYNC_FAILED,
    NTP_RTC_SYNCED
};

class NTPManager {
private:
    NTPStatus currentStatus;
    RTC_DS3231 rtc;
    bool rtcAvailable;
    bool ntpInitialized;
    bool timeIsSynced;
    unsigned long lastSyncAttempt;
    unsigned long lastRTCSync;
    unsigned long syncInterval;
    unsigned long rtcSyncInterval;
    
    // NTP configuration
    const long timezone = 7; // GMT+7 for Indonesia
    const int daylightSavingTime = 0;
    const char* ntpServer1 = "pool.ntp.org";
    const char* ntpServer2 = "time.nist.gov";
    const char* ntpServer3 = "time.google.com";
    
    // Non-blocking sync state
    bool syncInProgress;
    unsigned long syncStartTime;
    const unsigned long SYNC_TIMEOUT = 10000; // 10 second timeout
    
    // Private helper methods
    bool initializeRTC();
    bool attemptNTPSync();
    bool syncRTCWithNTP();
    bool isWiFiConnected();
    String formatDateTime(const DateTime& dt);
    String formatTime(const struct tm& timeinfo);
    
public:
    NTPManager();
    
    // Main interface
    bool initialize();
    void handle();
    bool isInitialized();
    bool isSynced();
    
    // Time operations
    DateTime getCurrentTime();
    String getCurrentTimeString();
    String getCurrentDateTimeString();
    time_t getCurrentUnixTime();
    bool setSystemTime(const DateTime& dt);
    
    // RTC operations
    bool isRTCAvailable();
    DateTime getRTCTime();
    bool setRTCTime(const DateTime& dt);
    float getRTCTemperature();
    
    // Sync operations
    bool requestSync();
    bool forceSyncRTC();
    unsigned long getLastSyncTime();
    unsigned long timeSinceLastSync();
    
    // Status and info
    NTPStatus getStatus();
    String getStatusString();
    String getTimeInfo();
    bool needsSync();
};

// Global instance
extern NTPManager ntpMgr;

#endif // NTP_MANAGER_H