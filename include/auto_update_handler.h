#ifndef AUTO_UPDATE_HANDLER_H
#define AUTO_UPDATE_HANDLER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <Ticker.h>
#include <Preferences.h>

#include "config.h"

// Auto-update status codes
enum AutoUpdateStatus {
    AUTO_UPDATE_IDLE = 0,
    AUTO_UPDATE_CHECKING = 1,
    AUTO_UPDATE_AVAILABLE = 2,
    AUTO_UPDATE_DOWNLOADING = 3,
    AUTO_UPDATE_INSTALLING = 4,
    AUTO_UPDATE_SUCCESS = 5,
    AUTO_UPDATE_ERROR = 6,
    AUTO_UPDATE_DISABLED = 7
};

// Auto-update error codes
enum AutoUpdateError {
    AUTO_UPDATE_ERROR_NONE = 0,
    AUTO_UPDATE_ERROR_NETWORK = 1,
    AUTO_UPDATE_ERROR_SERVER = 2,
    AUTO_UPDATE_ERROR_VERSION_FORMAT = 3,
    AUTO_UPDATE_ERROR_DOWNLOAD_FAILED = 4,
    AUTO_UPDATE_ERROR_INSTALL_FAILED = 5,
    AUTO_UPDATE_ERROR_CONFIG = 6
};

// Version information structure
struct VersionInfo {
    String version;
    String buildDate;
    String buildTime;
    String gitHash;
    size_t firmwareSize;
    String checksum;
    String downloadUrl;
    String releaseNotes;
};

// Auto-update configuration structure
struct AutoUpdateConfig {
    bool enabled;
    String serverUrl;
    unsigned long checkInterval;
    String versionEndpoint;
    String firmwareEndpoint;
    int maxRetry;
    unsigned long retryDelay;
};

class AutoUpdateHandler {
private:
    Ticker updateChecker;
    Preferences preferences;
    AutoUpdateConfig config;
    AutoUpdateStatus currentStatus;
    AutoUpdateError lastError;
    VersionInfo currentVersion;
    VersionInfo latestVersion;
    unsigned long lastCheckTime;
    unsigned long lastUpdateTime;
    int retryCount;
    bool updateAvailable;
    HTTPClient httpClient;
    
    // Helper functions
    void loadConfig();
    void saveConfig();
    bool compareVersions(const String& current, const String& latest);
    String getCurrentVersion();
    bool fetchVersionInfo();
    bool downloadFirmware();
    bool installFirmware(uint8_t* firmwareData, size_t size);
    void scheduleNextCheck();
    static void checkForUpdatesStatic();
    void checkForUpdates();
    String calculateChecksum(uint8_t* data, size_t size);
    bool verifyFirmware(uint8_t* data, size_t size, const String& expectedChecksum);
    
public:
    AutoUpdateHandler();
    
    // Main functions
    bool initialize();
    void begin();
    void handle();
    void end();
    
    // Configuration functions
    bool setConfig(const AutoUpdateConfig& newConfig);
    AutoUpdateConfig getConfig();
    bool setServerUrl(const String& url);
    String getServerUrl();
    bool setCheckInterval(unsigned long interval);
    unsigned long getCheckInterval();
    void enable();
    void disable();
    bool isEnabled();
    
    // Update functions
    bool checkNow();
    bool installNow();
    void cancelUpdate();
    
    // Status functions
    AutoUpdateStatus getStatus();
    AutoUpdateError getLastError();
    String getStatusString();
    String getErrorString();
    VersionInfo getCurrentVersionInfo();
    VersionInfo getLatestVersionInfo();
    bool isUpdateAvailable();
    unsigned long getLastCheckTime();
    unsigned long getLastUpdateTime();
    int getRetryCount();
    
    // Statistics
    struct UpdateStats {
        unsigned long totalChecks;
        unsigned long successfulChecks;
        unsigned long failedChecks;
        unsigned long totalUpdates;
        unsigned long successfulUpdates;
        unsigned long failedUpdates;
        unsigned long bytesDownloaded;
    };
    
    UpdateStats getStats();
    void resetStats();
};

// Global instance
extern AutoUpdateHandler autoUpdateHandler;

#endif // AUTO_UPDATE_HANDLER_H