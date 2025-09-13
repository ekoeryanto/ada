#include "ntp_manager.h"

// Global instance
NTPManager ntpMgr;

NTPManager::NTPManager() {
    currentStatus = NTP_NOT_INITIALIZED;
    rtcAvailable = false;
    ntpInitialized = false;
    timeIsSynced = false;
    lastSyncAttempt = 0;
    lastRTCSync = 0;
    syncInterval = 3600000; // Sync every hour
    rtcSyncInterval = 86400000; // Sync RTC daily
    syncInProgress = false;
    syncStartTime = 0;
}

bool NTPManager::initialize() {
    // Serial.println("[NTP] Initializing NTP Manager...");
    
    // Only initialize if WiFi is connected
    if (!isWiFiConnected()) {
        // Serial.println("[NTP] WiFi not connected - deferring NTP initialization");
        currentStatus = NTP_NOT_INITIALIZED;
        return false;
    }
    
    currentStatus = NTP_INITIALIZING;
    
    // Initialize RTC first
    if (initializeRTC()) {
        // Serial.println("[NTP] RTC initialized successfully");
        rtcAvailable = true;
    } else {
        // Serial.println("[NTP] RTC not available - continuing with NTP only");
        rtcAvailable = false;
    }
    
    // Configure NTP
    // Serial.println("[NTP] Configuring NTP servers...");
    configTime(3600 * timezone, daylightSavingTime * 3600, ntpServer1, ntpServer2, ntpServer3);
    
    ntpInitialized = true;
    currentStatus = NTP_SYNCING;
    
    // Start first sync attempt
    requestSync();
    
    // Serial.println("[NTP] NTP Manager initialized - sync in progress");
    return true;
}

void NTPManager::handle() {
    // Only handle if WiFi is connected
    if (!isWiFiConnected()) {
        if (currentStatus != NTP_NOT_INITIALIZED) {
            // Serial.println("[NTP] WiFi disconnected - pausing NTP operations");
            currentStatus = NTP_NOT_INITIALIZED;
            ntpInitialized = false;
        }
        return;
    }
    
    // Initialize if not already done and WiFi is connected
    if (!ntpInitialized) {
        initialize();
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Handle ongoing sync
    if (syncInProgress) {
        if (currentTime - syncStartTime > SYNC_TIMEOUT) {
            // Serial.println("[NTP] Sync timeout - marking as failed");
            syncInProgress = false;
            currentStatus = NTP_SYNC_FAILED;
        } else {
            // Check if sync completed
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) { // Non-blocking check
                if (timeinfo.tm_year > (2020 - 1900)) { // Valid year
                    syncInProgress = false;
                    timeIsSynced = true;
                    currentStatus = NTP_SYNCED;
                    lastSyncAttempt = currentTime;
                    
                    // Serial.printf("[NTP] Time synchronized: %s\n", formatTime(timeinfo).c_str());
                    
                    // Sync RTC if available
                    if (rtcAvailable) {
                        syncRTCWithNTP();
                    }
                }
            }
        }
    }
    
    // Periodic sync check
    if (timeIsSynced && (currentTime - lastSyncAttempt > syncInterval)) {
        // Serial.println("[NTP] Performing periodic sync...");
        requestSync();
    }
    
    // Periodic RTC sync
    if (rtcAvailable && timeIsSynced && (currentTime - lastRTCSync > rtcSyncInterval)) {
        // Serial.println("[NTP] Performing periodic RTC sync...");
        syncRTCWithNTP();
    }
}

bool NTPManager::isInitialized() {
    return ntpInitialized && isWiFiConnected();
}

bool NTPManager::isSynced() {
    return timeIsSynced && (currentStatus == NTP_SYNCED || currentStatus == NTP_RTC_SYNCED);
}

DateTime NTPManager::getCurrentTime() {
    if (rtcAvailable) {
        return rtc.now();
    } else if (timeIsSynced) {
        time_t now = time(nullptr);
        return DateTime((uint32_t)now);
    } else {
        return DateTime(2024, 1, 1, 0, 0, 0); // Default time
    }
}

String NTPManager::getCurrentTimeString() {
    DateTime now = getCurrentTime();
    return formatDateTime(now);
}

String NTPManager::getCurrentDateTimeString() {
    DateTime now = getCurrentTime();
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", 
             now.year(), now.month(), now.day(), 
             now.hour(), now.minute(), now.second());
    return String(buffer);
}

time_t NTPManager::getCurrentUnixTime() {
    if (timeIsSynced) {
        return time(nullptr);
    } else if (rtcAvailable) {
        return rtc.now().unixtime();
    } else {
        return 0;
    }
}

bool NTPManager::setSystemTime(const DateTime& dt) {
    if (!isInitialized()) return false;
    
    struct timeval tv;
    tv.tv_sec = dt.unixtime();
    tv.tv_usec = 0;
    
    if (settimeofday(&tv, nullptr) == 0) {
        timeIsSynced = true;
        currentStatus = NTP_SYNCED;
        return true;
    }
    return false;
}

bool NTPManager::isRTCAvailable() {
    return rtcAvailable;
}

DateTime NTPManager::getRTCTime() {
    if (rtcAvailable) {
        return rtc.now();
    }
    return DateTime(2024, 1, 1, 0, 0, 0);
}

bool NTPManager::setRTCTime(const DateTime& dt) {
    if (!rtcAvailable) return false;
    
    rtc.adjust(dt);
    // Serial.printf("[NTP] RTC time set to: %s\n", formatDateTime(dt).c_str());
    return true;
}

float NTPManager::getRTCTemperature() {
    if (rtcAvailable) {
        return rtc.getTemperature();
    }
    return 0.0;
}

bool NTPManager::requestSync() {
    if (!isWiFiConnected() || syncInProgress) {
        return false;
    }
    
    // Serial.println("[NTP] Starting NTP sync...");
    syncInProgress = true;
    syncStartTime = millis();
    currentStatus = NTP_SYNCING;
    
    // Trigger NTP sync
    configTime(3600 * timezone, daylightSavingTime * 3600, ntpServer1, ntpServer2, ntpServer3);
    
    return true;
}

bool NTPManager::forceSyncRTC() {
    if (!rtcAvailable || !timeIsSynced) {
        return false;
    }
    
    return syncRTCWithNTP();
}

unsigned long NTPManager::getLastSyncTime() {
    return lastSyncAttempt;
}

unsigned long NTPManager::timeSinceLastSync() {
    if (lastSyncAttempt == 0) return ULONG_MAX;
    return millis() - lastSyncAttempt;
}

NTPStatus NTPManager::getStatus() {
    return currentStatus;
}

String NTPManager::getStatusString() {
    switch (currentStatus) {
        case NTP_NOT_INITIALIZED: return "Not Initialized";
        case NTP_INITIALIZING: return "Initializing";
        case NTP_SYNCING: return "Syncing";
        case NTP_SYNCED: return "Synced";
        case NTP_SYNC_FAILED: return "Sync Failed";
        case NTP_RTC_SYNCED: return "RTC Synced";
        default: return "Unknown";
    }
}

String NTPManager::getTimeInfo() {
    String info = "NTP Manager Status:\n";
    info += "  Status: " + getStatusString() + "\n";
    info += "  WiFi Connected: " + String(isWiFiConnected() ? "Yes" : "No") + "\n";
    info += "  Time Synced: " + String(timeIsSynced ? "Yes" : "No") + "\n";
    info += "  RTC Available: " + String(rtcAvailable ? "Yes" : "No") + "\n";
    
    if (timeIsSynced || rtcAvailable) {
        info += "  Current Time: " + getCurrentDateTimeString() + "\n";
    }
    
    if (rtcAvailable) {
        info += "  RTC Temperature: " + String(getRTCTemperature(), 1) + "°C\n";
    }
    
    if (lastSyncAttempt > 0) {
        info += "  Last Sync: " + String((millis() - lastSyncAttempt) / 1000) + "s ago\n";
    }
    
    return info;
}

bool NTPManager::needsSync() {
    if (!timeIsSynced) return true;
    if (timeSinceLastSync() > syncInterval) return true;
    return false;
}

// Private helper methods
bool NTPManager::initializeRTC() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    if (!rtc.begin()) {
        // Serial.println("[NTP] RTC DS3231 not found");
        return false;
    }
    
    if (rtc.lostPower()) {
        // Serial.println("[NTP] RTC lost power - will sync with NTP");
    }
    
    return true;
}

bool NTPManager::attemptNTPSync() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5000)) {
        return false;
    }
    
    if (timeinfo.tm_year < (2020 - 1900)) {
        return false;
    }
    
    timeIsSynced = true;
    return true;
}

bool NTPManager::syncRTCWithNTP() {
    if (!rtcAvailable || !timeIsSynced) {
        return false;
    }
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 1000)) {
        // Serial.println("[NTP] Failed to get NTP time for RTC sync");
        return false;
    }
    
    // Convert tm to DateTime
    DateTime ntpTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    rtc.adjust(ntpTime);
    lastRTCSync = millis();
    currentStatus = NTP_RTC_SYNCED;
    
    // Serial.printf("[NTP] RTC synchronized with NTP time: %s\n", formatDateTime(ntpTime).c_str());
    return true;
}

bool NTPManager::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String NTPManager::formatDateTime(const DateTime& dt) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", 
             dt.year(), dt.month(), dt.day(), 
             dt.hour(), dt.minute(), dt.second());
    return String(buffer);
}

String NTPManager::formatTime(const struct tm& timeinfo) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buffer);
}