#include "sd_manager.h"
#include "ntp_manager.h"
#include "webhook_handler.h"
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
    bufferLoggingEnabled = true; // Enable buffer logging by default
    logBuffer.reserve(MAX_BUFFER_SIZE); // Pre-allocate memory
}

bool SDManager::initialize() {
    Serial.println("[SD] Initializing SD Manager...");
    
    // Use default ESP32 SPI pins (like sample code)
    // Default pins: CS=5, MOSI=23, MISO=19, SCK=18
    // Just initialize SD card with default pins
    
    // Reset state first
    sdInitialized = false;
    sdCardPresent = false;
    cardType = CARD_NONE;
    cardSize = 0;
    
    try {
        if (!SD.begin()) {
            Serial.println("[SD] Card Mount Failed - continuing without SD logging");
            Serial.println("[SD] System will operate normally without SD card");
            return false;
        }
        
        // Check card presence with timeout protection
        unsigned long startTime = millis();
        while (millis() - startTime < 3000) {  // 3 second timeout
            cardType = SD.cardType();
            if (cardType != CARD_NONE) break;
            delay(100);
        }
        
        if (cardType == CARD_NONE) {
            Serial.println("[SD] No SD card detected - continuing without SD logging");
            Serial.println("[SD] Insert SD card and restart to enable logging");
            return false;
        }
        
        // Test card access - try to get card size
        uint64_t testSize = 0;
        try {
            testSize = SD.cardSize();
        } catch (...) {
            Serial.println("[SD] Card access failed - possible corruption");
            Serial.println("[SD] Try formatting the SD card or use a different one");
            return false;
        }
        
        if (testSize == 0) {
            Serial.println("[SD] Invalid card size - possible corruption");
            Serial.println("[SD] Try formatting the SD card or use a different one");
            return false;
        }
        
        cardSize = testSize / (1024 * 1024); // Convert to MB
        sdInitialized = true;
        sdCardPresent = true;
        
        Serial.printf("[SD] SD Manager initialized successfully\n");
        Serial.printf("[SD] Card Type: %s\n", getCardType().c_str());
        Serial.printf("[SD] Card Size: %llu MB\n", cardSize);
        Serial.println("[SD] Logging to SD card enabled");
        
        return true;
        
    } catch (const std::exception& e) {
        Serial.printf("[SD] Exception during initialization: %s\n", e.what());
        Serial.println("[SD] Continuing without SD logging");
        return false;
    } catch (...) {
        Serial.println("[SD] Unknown error during SD initialization");
        Serial.println("[SD] Continuing without SD logging");
        return false;
    }
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
                
                // Inform user about buffer logging
                if (bufferLoggingEnabled && DEBUG_ENABLED) {
                    Serial.printf("[SD] Logging will continue in memory buffer (%d entries max)\n", MAX_BUFFER_SIZE);
                }
                
            } else if (currentType != CARD_NONE && !sdCardPresent) {
                Serial.println("[SD] Card inserted - attempting remount");
                if (initialize()) {
                    // Card reinserted successfully - try to flush buffer
                    if (!logBuffer.empty()) {
                        Serial.printf("[SD] Attempting to flush %d buffered entries to SD\n", logBuffer.size());
                        flushBufferToSD();
                    }
                }
            }
        } else {
            // SD not initialized - periodically try to reinitialize in case card was inserted
            if (currentTime % 30000 == 0) { // Every 30 seconds
                initialize();
            }
        }
        
        // Buffer management - warn if buffer is getting full
        if (bufferLoggingEnabled && logBuffer.size() > (MAX_BUFFER_SIZE * 0.8)) {
            if (DEBUG_ENABLED) {
                Serial.printf("[SD] Warning: Log buffer is %d%% full (%d/%d entries)\n", 
                             (int)((logBuffer.size() * 100) / MAX_BUFFER_SIZE), 
                             logBuffer.size(), MAX_BUFFER_SIZE);
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

// Helper method to get current date string for file naming
String SDManager::getDateString() {
    extern NTPManager ntpMgr;
    if (ntpMgr.isSynced()) {
        // Extract date part from datetime string (assume format: YYYY-MM-DD HH:MM:SS)
        String datetime = ntpMgr.getCurrentTimeString();
        int spaceIndex = datetime.indexOf(' ');
        if (spaceIndex > 0) {
            return datetime.substring(0, spaceIndex); // Return YYYY-MM-DD part
        }
        return "date_" + String(millis() / 86400000); // Fallback
    } else {
        // Fallback to simple millis-based naming
        return "day_" + String(millis() / 86400000); // Days since startup
    }
}

bool SDManager::logDataWithTimestamp(const String& data) {
    // Get current timestamp from NTP Manager
    String timestamp;
    extern NTPManager ntpMgr; // Forward reference
    if (ntpMgr.isSynced()) {
        timestamp = ntpMgr.getCurrentDateTimeString();
    } else {
        // Fallback to millis if NTP not available
        timestamp = String(millis());
    }
    
    // Create timestamped log entry
    String logEntry = timestamp + "," + data;
    
    // Try to write to SD card first
    if (isMounted()) {
        String filename = "data_" + getDateString() + ".csv";
        File logFile = SD.open("/" + filename, FILE_APPEND);
        
        if (logFile) {
            logFile.println(logEntry);
            logFile.close();
            
            // If SD write successful and we have buffered data, try to flush buffer
            if (!logBuffer.empty()) {
                flushBufferToSD();
            }
            
            return true;
        } else {
            if (DEBUG_ENABLED) {
                Serial.println("[SD] Failed to open log file: " + filename);
            }
        }
    }
    
    // SD not available or write failed - use buffer if enabled
    if (bufferLoggingEnabled) {
        addToBuffer(logEntry);
        if (DEBUG_ENABLED) {
            Serial.printf("[SD] Logged to buffer (%d/%d): %s\n", 
                         logBuffer.size(), MAX_BUFFER_SIZE, data.c_str());
        }
        return true; // Consider buffer logging as success
    }
    
    return false; // Both SD and buffer failed/disabled
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
    extern NTPManager ntpMgr; // Forward reference
    if (ntpMgr.isSynced()) {
        return ntpMgr.getCurrentDateTimeString();
    } else {
        // Fallback to millis-based timestamp
        return String(millis());
    }
}

// Buffer management methods
void SDManager::addToBuffer(const String& logEntry) {
    if (!bufferLoggingEnabled) return;
    
    // Add new entry
    logBuffer.push_back(logEntry);
    
    // Keep buffer size under limit (FIFO - remove oldest)
    while (logBuffer.size() > MAX_BUFFER_SIZE) {
        logBuffer.erase(logBuffer.begin());
    }
}

void SDManager::flushBufferToSD() {
    if (!isMounted() || logBuffer.empty()) return;
    
    String filename = "data_" + getDateString() + ".csv";
    File logFile = SD.open("/" + filename, FILE_APPEND);
    
    if (!logFile) {
        if (DEBUG_ENABLED) {
            Serial.println("[SD] Failed to open log file for buffer flush");
        }
        return;
    }
    
    // Write all buffered entries
    size_t flushed = 0;
    for (const String& entry : logBuffer) {
        logFile.println(entry);
        flushed++;
    }
    
    logFile.close();
    
    if (DEBUG_ENABLED) {
        Serial.printf("[SD] Flushed %d entries from buffer to SD\n", flushed);
    }
    
    // Clear buffer after successful flush
    logBuffer.clear();
}

void SDManager::sendBufferToWebhook() {
    if (logBuffer.empty()) return;
    
    // Forward declare webhook handler
    extern WebhookHandler webhookHandler;
    
    if (!webhookHandler.isInitialized()) {
        if (DEBUG_ENABLED) {
            Serial.println("[SD] Webhook not available for buffer backup");
        }
        return;
    }
    
    // Send buffer as bulk data via webhook
    String bulkData = "";
    for (const String& entry : logBuffer) {
        if (bulkData.length() > 0) bulkData += "\n";
        bulkData += entry;
    }
    
    // Use webhook to send buffered data (implement this in webhook handler)
    // webhookHandler.sendBulkData("sd_buffer", bulkData);
    
    if (DEBUG_ENABLED) {
        Serial.printf("[SD] Attempted to send %d buffered entries via webhook\n", logBuffer.size());
    }
}

void SDManager::enableBufferLogging(bool enable) {
    bufferLoggingEnabled = enable;
    if (DEBUG_ENABLED) {
        Serial.printf("[SD] Buffer logging %s\n", enable ? "enabled" : "disabled");
    }
    
    if (!enable) {
        // Clear buffer when disabling
        logBuffer.clear();
    }
}

bool SDManager::isBufferLoggingEnabled() {
    return bufferLoggingEnabled;
}

size_t SDManager::getBufferSize() {
    return logBuffer.size();
}

std::vector<String> SDManager::getBufferContent() {
    return logBuffer; // Return copy
}

void SDManager::clearBuffer() {
    logBuffer.clear();
    if (DEBUG_ENABLED) {
        Serial.println("[SD] Log buffer cleared");
    }
}

bool SDManager::isOperationSafe() {
    return isMounted();
}