#include "sd_manager.h"
#include "ntp_manager.h"
#include <time.h>
#include <vector>

// Global instance
SDManager sdMgr;

SDManager::SDManager() {
    sdInitialized = false;
    sdCardPresent = false;
    cardSize = 0;
    cardType = CARD_NONE;
    lastCheck = 0;
}

bool SDManager::initialize() {
    Serial.println("[SD] Initializing SD Manager...");
    
    // Initialize SPI with custom pins - non-blocking
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    
    // Try to initialize SD card - if it fails, just continue without SD
    if (!SD.begin(SD_CS)) {
        Serial.println("[SD] Card Mount Failed - continuing without SD");
        sdInitialized = false;
        sdCardPresent = false;
        return false;
    }
    
    // Check card presence
    cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] No SD card attached - continuing without SD");
        sdInitialized = false;
        sdCardPresent = false;
        return false;
    }
    
    cardSize = SD.cardSize() / (1024 * 1024); // Convert to MB
    sdInitialized = true;
    sdCardPresent = true;
    
    Serial.printf("[SD] SD Manager initialized successfully\n");
    Serial.printf("[SD] Card Type: %s\n", getCardType().c_str());
    Serial.printf("[SD] Card Size: %llu MB\n", cardSize);
    
    return true;
}

void SDManager::handle() {
    // Non-blocking periodic check
    unsigned long currentTime = millis();
    
    if (currentTime - lastCheck >= CHECK_INTERVAL) {
        lastCheck = currentTime;
        
        // Quick presence check without blocking
        if (sdInitialized) {
            uint8_t currentType = SD.cardType();
            if (currentType == CARD_NONE && sdCardPresent) {
                Serial.println("[SD] Card removed - hot-plug detected");
                sdCardPresent = false;
                sdInitialized = false;
            } else if (currentType != CARD_NONE && !sdCardPresent) {
                Serial.println("[SD] Card inserted - attempting remount");
                initialize(); // Try to reinitialize
            }
        }
    }
}

bool SDManager::isMounted() {
    return sdInitialized && sdCardPresent;
}

bool SDManager::isCardPresent() {
    return sdCardPresent;
}

String SDManager::getCardType() {
    switch (cardType) {
        case CARD_MMC: return "MMC";
        case CARD_SD: return "SDSC";
        case CARD_SDHC: return "SDHC";
        case CARD_NONE: return "None";
        default: return "Unknown";
    }
}

uint64_t SDManager::getCardSize() {
    if (!isMounted()) return 0;
    return cardSize;
}

uint64_t SDManager::getTotalBytes() {
    if (!isMounted()) return 0;
    return SD.totalBytes();
}

uint64_t SDManager::getUsedBytes() {
    if (!isMounted()) return 0;
    return SD.usedBytes();
}

uint64_t SDManager::getFreeSpace() {
    if (!isMounted()) return 0;
    return SD.totalBytes() - SD.usedBytes();
}

String SDManager::getStatus() {
    if (!sdInitialized) {
        return "Not initialized";
    } else if (!sdCardPresent) {
        return "Card not present";
    } else {
        return "Ready";
    }
}

bool SDManager::runSelfTest() {
    if (!isMounted()) {
        Serial.println("[SD] Self-test failed - SD not available");
        return false;
    }
    
    Serial.println("[SD] Running basic self-test...");
    
    // Simple test - just try to open root directory
    File root = SD.open("/");
    if (!root) {
        Serial.println("[SD] Self-test failed - cannot open root directory");
        return false;
    }
    root.close();
    
    Serial.println("[SD] Self-test passed!");
    return true;
}

// Stub implementations for remaining methods to satisfy the interface
bool SDManager::writeFile(const char* path, const char* message) {
    if (!isMounted()) return false;
    // Implementation would go here
    return false;
}

bool SDManager::appendFile(const char* path, const char* message) {
    if (!isMounted()) return false;
    // Implementation would go here
    return false;
}

String SDManager::readFile(const char* path) {
    if (!isMounted()) return "";
    // Implementation would go here
    return "";
}

bool SDManager::deleteFile(const char* path) {
    if (!isMounted()) return false;
    // Implementation would go here
    return false;
}

bool SDManager::renameFile(const char* oldPath, const char* newPath) {
    if (!isMounted()) return false;
    // Implementation would go here
    return false;
}

bool SDManager::fileExists(const char* path) {
    if (!isMounted()) return false;
    return SD.exists(path);
}

size_t SDManager::getFileSize(const char* path) {
    if (!isMounted()) return 0;
    // Implementation would go here
    return 0;
}

std::vector<String> SDManager::listDirectory(const String& path) {
    std::vector<String> files;
    if (!isMounted()) return files;
    // Implementation would go here
    return files;
}

bool SDManager::createDir(const char* path) {
    if (!isMounted()) return false;
    // Implementation would go here
    return false;
}

bool SDManager::removeDir(const char* path) {
    if (!isMounted()) return false;
    // Implementation would go here
    return false;
}

bool SDManager::logData(const String& data) {
    if (!isMounted()) return false;
    
    // Simple data logging without timestamp
    String filename = "system.log";
    File logFile = SD.open("/" + filename, FILE_APPEND);
    
    if (!logFile) {
        if (DEBUG_ENABLED) {
            Serial.println("[SD] Failed to open log file: " + filename);
        }
        return false;
    }
    
    logFile.println(data);
    logFile.close();
    
    return true;
}

bool SDManager::logDataWithTimestamp(const String& data) {
    if (!isMounted()) return false;
    
    // Get current timestamp from NTP Manager
    String timestamp;
    if (ntpMgr.isSynced()) {
        timestamp = ntpMgr.getCurrentDateTimeString();
    } else {
        // Fallback to millis if NTP not available
        timestamp = String(millis());
    }
    
    // Create timestamped log entry
    String logEntry = timestamp + "," + data;
    
    // Write to daily log file
    String filename = "data_" + getDateString() + ".csv";
    File logFile = SD.open("/" + filename, FILE_APPEND);
    
    if (!logFile) {
        if (DEBUG_ENABLED) {
            Serial.println("[SD] Failed to open log file: " + filename);
        }
        return false;
    }
    
    logFile.println(logEntry);
    logFile.close();
    
    if (DEBUG_ENABLED) {
        Serial.println("[SD] Data logged: " + logEntry);
    }
    
    return true;
}

bool SDManager::createLogFile(const String& filename) {
    if (!isMounted()) return false;
    
    String logFilename = filename.isEmpty() ? generateLogFilename() : filename;
    
    // Create log file with CSV header if it doesn't exist
    if (!SD.exists("/" + logFilename)) {
        File logFile = SD.open("/" + logFilename, FILE_WRITE);
        if (!logFile) {
            if (DEBUG_ENABLED) {
                Serial.println("[SD] Failed to create log file: " + logFilename);
            }
            return false;
        }
        
        // Write CSV header
        logFile.println("timestamp,sensor_type,data");
        logFile.close();
        
        if (DEBUG_ENABLED) {
            Serial.println("[SD] Created log file: " + logFilename);
        }
    }
    
    return true;
}

String SDManager::generateLogFilename() {
    return "log_" + String(millis()) + ".txt";
}

String SDManager::getCardInfo() {
    if (!isMounted()) {
        return "SD card not available";
    }
    
    String info = "SD Card Information:\n";
    info += "  Type: " + getCardType() + "\n";
    info += "  Size: " + String(cardSize) + " MB\n";
    info += "  Status: " + getStatus() + "\n";
    
    return info;
}

bool SDManager::remount() {
    Serial.println("[SD] Attempting to remount SD card...");
    unmount();
    delay(100); // Brief delay
    return initialize();
}

void SDManager::unmount() {
    Serial.println("[SD] Unmounting SD card...");
    SD.end();
    sdInitialized = false;
    sdCardPresent = false;
}

// Private helper methods
void SDManager::detectCardType() {
    cardType = SD.cardType();
}

bool SDManager::checkCardPresence() {
    return (SD.cardType() != CARD_NONE);
}

String SDManager::formatBytes(uint64_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) return String(bytes / 1024.0, 2) + " KB";
    else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1048576.0, 2) + " MB";
    else return String(bytes / 1073741824.0, 2) + " GB";
}

String SDManager::getCurrentTimestamp() {
    // Use NTP Manager for proper timestamp if available
    if (ntpMgr.isSynced()) {
        return ntpMgr.getCurrentDateTimeString();
    } else {
        // Fallback to millis-based timestamp
        return String(millis());
    }
}

String SDManager::getDateString() {
    // Get current date for daily log files
    if (ntpMgr.isSynced()) {
        DateTime now = ntpMgr.getCurrentTime();
        char dateStr[16];
        snprintf(dateStr, sizeof(dateStr), "%04d%02d%02d", 
                now.year(), now.month(), now.day());
        return String(dateStr);
    } else {
        // Fallback to day based on millis
        return String(millis() / 86400000);  // Days since boot
    }
}

bool SDManager::isOperationSafe() {
    return isMounted();
}