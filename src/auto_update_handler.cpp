#include "auto_update_handler.h"

// Global instance
AutoUpdateHandler autoUpdateHandler;

AutoUpdateHandler::AutoUpdateHandler() {
    currentStatus = AUTO_UPDATE_IDLE;
    lastError = AUTO_UPDATE_ERROR_NONE;
    lastCheckTime = 0;
    lastUpdateTime = 0;
    retryCount = 0;
    updateAvailable = false;
    
    // Initialize current version
    currentVersion.version = PROJECT_VERSION;
    currentVersion.buildDate = __DATE__;
    currentVersion.buildTime = __TIME__;
    currentVersion.gitHash = ""; // Can be set during build
    currentVersion.firmwareSize = 0;
    currentVersion.checksum = "";
    currentVersion.downloadUrl = "";
    currentVersion.releaseNotes = "";
}

bool AutoUpdateHandler::initialize() {
    Serial.println("[AutoUpdate] Initializing auto-update handler");
    
    // Initialize preferences
    if (!preferences.begin("autoupdate", false)) {
        Serial.println("[AutoUpdate] Failed to initialize preferences");
        return false;
    }
    
    // Load configuration
    loadConfig();
    
    Serial.printf("[AutoUpdate] Configuration loaded: enabled=%s, server=%s, interval=%lu\n", 
                  config.enabled ? "true" : "false", 
                  config.serverUrl.c_str(), 
                  config.checkInterval);
    
    return true;
}

void AutoUpdateHandler::loadConfig() {
    // Load configuration from preferences or use defaults
    config.enabled = preferences.getBool("enabled", AUTO_UPDATE_ENABLED);
    config.serverUrl = preferences.getString("serverUrl", AUTO_UPDATE_SERVER_URL);
    config.checkInterval = preferences.getULong("checkInterval", AUTO_UPDATE_CHECK_INTERVAL);
    config.versionEndpoint = preferences.getString("versionEndpoint", AUTO_UPDATE_VERSION_ENDPOINT);
    config.firmwareEndpoint = preferences.getString("firmwareEndpoint", AUTO_UPDATE_FIRMWARE_ENDPOINT);
    config.maxRetry = preferences.getInt("maxRetry", AUTO_UPDATE_MAX_RETRY);
    config.retryDelay = preferences.getULong("retryDelay", AUTO_UPDATE_RETRY_DELAY);
}

void AutoUpdateHandler::saveConfig() {
    preferences.putBool("enabled", config.enabled);
    preferences.putString("serverUrl", config.serverUrl);
    preferences.putULong("checkInterval", config.checkInterval);
    preferences.putString("versionEndpoint", config.versionEndpoint);
    preferences.putString("firmwareEndpoint", config.firmwareEndpoint);
    preferences.putInt("maxRetry", config.maxRetry);
    preferences.putULong("retryDelay", config.retryDelay);
}

void AutoUpdateHandler::begin() {
    if (!config.enabled) {
        currentStatus = AUTO_UPDATE_DISABLED;
        Serial.println("[AutoUpdate] Auto-update is disabled");
        return;
    }
    
    Serial.println("[AutoUpdate] Starting auto-update service");
    currentStatus = AUTO_UPDATE_IDLE;
    
    // Schedule first check
    scheduleNextCheck();
}

void AutoUpdateHandler::scheduleNextCheck() {
    if (!config.enabled) return;
    
    updateChecker.once_ms(config.checkInterval, checkForUpdatesStatic);
    Serial.printf("[AutoUpdate] Next check scheduled in %lu ms\n", config.checkInterval);
}

void AutoUpdateHandler::checkForUpdatesStatic() {
    autoUpdateHandler.checkForUpdates();
}

void AutoUpdateHandler::checkForUpdates() {
    if (!config.enabled || WiFi.status() != WL_CONNECTED) {
        Serial.println("[AutoUpdate] Skipping check: disabled or no WiFi");
        scheduleNextCheck();
        return;
    }
    
    Serial.println("[AutoUpdate] Checking for updates...");
    currentStatus = AUTO_UPDATE_CHECKING;
    lastCheckTime = millis();
    
    if (fetchVersionInfo()) {
        if (compareVersions(currentVersion.version, latestVersion.version)) {
            Serial.printf("[AutoUpdate] New version available: %s -> %s\n", 
                         currentVersion.version.c_str(), 
                         latestVersion.version.c_str());
            
            currentStatus = AUTO_UPDATE_AVAILABLE;
            updateAvailable = true;
            
            // Auto-install if enabled (could be configurable)
            if (downloadFirmware()) {
                Serial.println("[AutoUpdate] Update completed successfully");
                currentStatus = AUTO_UPDATE_SUCCESS;
                lastUpdateTime = millis();
                retryCount = 0;
                
                // Restart in 5 seconds
                delay(5000);
                ESP.restart();
            } else {
                Serial.println("[AutoUpdate] Update failed");
                currentStatus = AUTO_UPDATE_ERROR;
                retryCount++;
                
                if (retryCount < config.maxRetry) {
                    Serial.printf("[AutoUpdate] Retry %d/%d in %lu ms\n", 
                                 retryCount, config.maxRetry, config.retryDelay);
                    updateChecker.once_ms(config.retryDelay, checkForUpdatesStatic);
                    return;
                }
            }
        } else {
            Serial.println("[AutoUpdate] No updates available");
            currentStatus = AUTO_UPDATE_IDLE;
            updateAvailable = false;
            retryCount = 0;
        }
    } else {
        Serial.println("[AutoUpdate] Failed to check for updates");
        currentStatus = AUTO_UPDATE_ERROR;
        retryCount++;
        
        if (retryCount < config.maxRetry) {
            Serial.printf("[AutoUpdate] Retry %d/%d in %lu ms\n", 
                         retryCount, config.maxRetry, config.retryDelay);
            updateChecker.once_ms(config.retryDelay, checkForUpdatesStatic);
            return;
        }
    }
    
    // Schedule next regular check
    scheduleNextCheck();
}

bool AutoUpdateHandler::fetchVersionInfo() {
    if (!WiFi.isConnected()) {
        lastError = AUTO_UPDATE_ERROR_NETWORK;
        return false;
    }
    
    httpClient.begin(config.serverUrl + config.versionEndpoint);
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("User-Agent", String("ESP32-") + HOSTNAME + "/" + PROJECT_VERSION);
    
    // Add current version info for server reference
    DynamicJsonDocument requestDoc(512);
    requestDoc["current_version"] = currentVersion.version;
    requestDoc["device_id"] = WiFi.macAddress();
    requestDoc["hostname"] = HOSTNAME;
    requestDoc["project"] = PROJECT_NAME;
    
    String requestPayload;
    serializeJson(requestDoc, requestPayload);
    
    int httpResponseCode = httpClient.POST(requestPayload);
    
    if (httpResponseCode == 200) {
        String payload = httpClient.getString();
        
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.printf("[AutoUpdate] JSON parse error: %s\n", error.c_str());
            lastError = AUTO_UPDATE_ERROR_VERSION_FORMAT;
            httpClient.end();
            return false;
        }
        
        // Parse version information
        latestVersion.version = doc["version"].as<String>();
        latestVersion.buildDate = doc["build_date"].as<String>();
        latestVersion.buildTime = doc["build_time"].as<String>();
        latestVersion.gitHash = doc["git_hash"].as<String>();
        latestVersion.firmwareSize = doc["firmware_size"];
        latestVersion.checksum = doc["checksum"].as<String>();
        latestVersion.downloadUrl = doc["download_url"].as<String>();
        latestVersion.releaseNotes = doc["release_notes"].as<String>();
        
        Serial.printf("[AutoUpdate] Latest version: %s, size: %zu bytes\n", 
                     latestVersion.version.c_str(), 
                     latestVersion.firmwareSize);
        
        httpClient.end();
        return true;
    } else {
        Serial.printf("[AutoUpdate] HTTP error: %d\n", httpResponseCode);
        lastError = AUTO_UPDATE_ERROR_SERVER;
        httpClient.end();
        return false;
    }
}

bool AutoUpdateHandler::compareVersions(const String& current, const String& latest) {
    // Simple version comparison (major.minor.patch)
    // Returns true if latest > current
    
    if (current == latest) return false;
    
    // Parse version numbers
    int currentParts[3] = {0, 0, 0};
    int latestParts[3] = {0, 0, 0};
    
    // Parse current version
    int partIndex = 0;
    String temp = "";
    for (int i = 0; i < current.length() && partIndex < 3; i++) {
        if (current[i] == '.') {
            currentParts[partIndex++] = temp.toInt();
            temp = "";
        } else {
            temp += current[i];
        }
    }
    if (partIndex < 3 && temp.length() > 0) {
        currentParts[partIndex] = temp.toInt();
    }
    
    // Parse latest version
    partIndex = 0;
    temp = "";
    for (int i = 0; i < latest.length() && partIndex < 3; i++) {
        if (latest[i] == '.') {
            latestParts[partIndex++] = temp.toInt();
            temp = "";
        } else {
            temp += latest[i];
        }
    }
    if (partIndex < 3 && temp.length() > 0) {
        latestParts[partIndex] = temp.toInt();
    }
    
    // Compare versions
    for (int i = 0; i < 3; i++) {
        if (latestParts[i] > currentParts[i]) return true;
        if (latestParts[i] < currentParts[i]) return false;
    }
    
    return false; // Versions are equal
}

bool AutoUpdateHandler::downloadFirmware() {
    if (latestVersion.downloadUrl.isEmpty()) {
        Serial.println("[AutoUpdate] No download URL provided");
        lastError = AUTO_UPDATE_ERROR_CONFIG;
        return false;
    }
    
    Serial.printf("[AutoUpdate] Downloading firmware from: %s\n", latestVersion.downloadUrl.c_str());
    currentStatus = AUTO_UPDATE_DOWNLOADING;
    
    httpClient.begin(latestVersion.downloadUrl);
    
    int httpResponseCode = httpClient.GET();
    
    if (httpResponseCode == 200) {
        int contentLength = httpClient.getSize();
        
        if (contentLength != latestVersion.firmwareSize) {
            Serial.printf("[AutoUpdate] Size mismatch: expected %zu, got %d\n", 
                         latestVersion.firmwareSize, contentLength);
            lastError = AUTO_UPDATE_ERROR_DOWNLOAD_FAILED;
            httpClient.end();
            return false;
        }
        
        Serial.printf("[AutoUpdate] Starting OTA update, size: %d bytes\n", contentLength);
        currentStatus = AUTO_UPDATE_INSTALLING;
        
        if (!Update.begin(contentLength)) {
            Serial.println("[AutoUpdate] Not enough space for OTA");
            lastError = AUTO_UPDATE_ERROR_INSTALL_FAILED;
            httpClient.end();
            return false;
        }
        
        WiFiClient* client = httpClient.getStreamPtr();
        size_t written = Update.writeStream(*client);
        
        if (written == contentLength) {
            Serial.println("[AutoUpdate] Firmware written successfully");
        } else {
            Serial.printf("[AutoUpdate] Write error: %zu/%d bytes\n", written, contentLength);
            lastError = AUTO_UPDATE_ERROR_INSTALL_FAILED;
            httpClient.end();
            return false;
        }
        
        if (Update.end()) {
            if (Update.isFinished()) {
                Serial.println("[AutoUpdate] OTA update finished successfully");
                httpClient.end();
                return true;
            } else {
                Serial.println("[AutoUpdate] OTA update not finished");
                lastError = AUTO_UPDATE_ERROR_INSTALL_FAILED;
            }
        } else {
            Serial.printf("[AutoUpdate] OTA error: %s\n", Update.errorString());
            lastError = AUTO_UPDATE_ERROR_INSTALL_FAILED;
        }
    } else {
        Serial.printf("[AutoUpdate] Download failed, HTTP code: %d\n", httpResponseCode);
        lastError = AUTO_UPDATE_ERROR_DOWNLOAD_FAILED;
    }
    
    httpClient.end();
    return false;
}

// Configuration functions
bool AutoUpdateHandler::setConfig(const AutoUpdateConfig& newConfig) {
    config = newConfig;
    saveConfig();
    
    if (config.enabled) {
        begin();
    } else {
        end();
    }
    
    return true;
}

AutoUpdateConfig AutoUpdateHandler::getConfig() {
    return config;
}

bool AutoUpdateHandler::setServerUrl(const String& url) {
    config.serverUrl = url;
    saveConfig();
    return true;
}

String AutoUpdateHandler::getServerUrl() {
    return config.serverUrl;
}

bool AutoUpdateHandler::setCheckInterval(unsigned long interval) {
    config.checkInterval = interval;
    saveConfig();
    
    // Reschedule if running
    if (config.enabled) {
        updateChecker.detach();
        scheduleNextCheck();
    }
    
    return true;
}

unsigned long AutoUpdateHandler::getCheckInterval() {
    return config.checkInterval;
}

void AutoUpdateHandler::enable() {
    config.enabled = true;
    saveConfig();
    begin();
}

void AutoUpdateHandler::disable() {
    config.enabled = false;
    saveConfig();
    end();
}

bool AutoUpdateHandler::isEnabled() {
    return config.enabled;
}

void AutoUpdateHandler::end() {
    updateChecker.detach();
    currentStatus = AUTO_UPDATE_DISABLED;
    Serial.println("[AutoUpdate] Auto-update service stopped");
}

void AutoUpdateHandler::handle() {
    // Handle any periodic tasks if needed
    // Most work is done by the Ticker callback
}

// Status functions
AutoUpdateStatus AutoUpdateHandler::getStatus() {
    return currentStatus;
}

AutoUpdateError AutoUpdateHandler::getLastError() {
    return lastError;
}

String AutoUpdateHandler::getStatusString() {
    switch (currentStatus) {
        case AUTO_UPDATE_IDLE: return "idle";
        case AUTO_UPDATE_CHECKING: return "checking";
        case AUTO_UPDATE_AVAILABLE: return "available";
        case AUTO_UPDATE_DOWNLOADING: return "downloading";
        case AUTO_UPDATE_INSTALLING: return "installing";
        case AUTO_UPDATE_SUCCESS: return "success";
        case AUTO_UPDATE_ERROR: return "error";
        case AUTO_UPDATE_DISABLED: return "disabled";
        default: return "unknown";
    }
}

String AutoUpdateHandler::getErrorString() {
    switch (lastError) {
        case AUTO_UPDATE_ERROR_NONE: return "none";
        case AUTO_UPDATE_ERROR_NETWORK: return "network_error";
        case AUTO_UPDATE_ERROR_SERVER: return "server_error";
        case AUTO_UPDATE_ERROR_VERSION_FORMAT: return "version_format_error";
        case AUTO_UPDATE_ERROR_DOWNLOAD_FAILED: return "download_failed";
        case AUTO_UPDATE_ERROR_INSTALL_FAILED: return "install_failed";
        case AUTO_UPDATE_ERROR_CONFIG: return "config_error";
        default: return "unknown_error";
    }
}

VersionInfo AutoUpdateHandler::getCurrentVersionInfo() {
    return currentVersion;
}

VersionInfo AutoUpdateHandler::getLatestVersionInfo() {
    return latestVersion;
}

bool AutoUpdateHandler::isUpdateAvailable() {
    return updateAvailable;
}

unsigned long AutoUpdateHandler::getLastCheckTime() {
    return lastCheckTime;
}

unsigned long AutoUpdateHandler::getLastUpdateTime() {
    return lastUpdateTime;
}

int AutoUpdateHandler::getRetryCount() {
    return retryCount;
}

bool AutoUpdateHandler::checkNow() {
    if (!config.enabled) return false;
    
    // Cancel scheduled check and do immediate check
    updateChecker.detach();
    checkForUpdates();
    return true;
}

bool AutoUpdateHandler::installNow() {
    if (!updateAvailable) return false;
    
    return downloadFirmware();
}

void AutoUpdateHandler::cancelUpdate() {
    updateChecker.detach();
    currentStatus = AUTO_UPDATE_IDLE;
    updateAvailable = false;
    retryCount = 0;
    
    if (config.enabled) {
        scheduleNextCheck();
    }
}