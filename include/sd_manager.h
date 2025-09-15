#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <vector>
#include "pins_config.h"
#include "config.h"

class SDManager {
private:
    bool sdInitialized;
    bool sdCardPresent;
    uint64_t cardSize;
    uint8_t cardType;
    unsigned long lastCheck;
    const unsigned long CHECK_INTERVAL = 5000; // Check every 5 seconds
    const unsigned long OPERATION_TIMEOUT = 2000; // 2 second timeout for operations
    
    // Fallback logging buffer when SD is unavailable
    std::vector<String> logBuffer;
    const size_t MAX_BUFFER_SIZE = 100; // Keep last 100 log entries in memory
    bool bufferLoggingEnabled;
    
    // Private helper methods
    void detectCardType();
    bool checkCardPresence();
    String formatBytes(uint64_t bytes);
    String getCurrentTimestamp();
    String getDateString();
    bool isOperationSafe();
    void addToBuffer(const String& logEntry);
    void flushBufferToSD();
    void sendBufferToWebhook();
    bool createLogFile(const String& filename);
    String generateLogFilename();
    
public:
    SDManager();
    
    // Initialization and management
    bool initialize();
    void handle();
    bool isMounted();
    bool isCardPresent();
    
    // File operations
    bool writeFile(const char* path, const char* message);
    bool appendFile(const char* path, const char* message);
    String readFile(const char* path);
    bool deleteFile(const char* path);
    bool renameFile(const char* oldPath, const char* newPath);
    bool fileExists(const char* path);
    size_t getFileSize(const char* path);
    std::vector<String> listDirectory(const String& path);
    
    // Directory operations
    bool createDir(const char* path);
    bool removeDir(const char* path);
    
    // Data logging methods (safe with fallback)
    bool logDataWithTimestamp(const String& data);
    bool logError(const String& error);
    bool logDebug(const String& debug);
    
    // Buffer management
    void enableBufferLogging(bool enable = true);
    bool isBufferLoggingEnabled();
    size_t getBufferSize();
    std::vector<String> getBufferContent();
    void clearBuffer();
    // Note: flushBufferToSD() and sendBufferToWebhook() are private methods
    
    // System information
    String getCardInfo();
    String getCardType();
    uint64_t getCardSize();
    uint64_t getTotalBytes();
    uint64_t getUsedBytes();
    uint64_t getFreeSpace();
    
    // Hot-plug support
    bool remount();
    void unmount();
    
    // Status and testing methods
    String getStatus();
    bool runSelfTest();
};

// Global instance
extern SDManager sdMgr;

#endif // SD_MANAGER_H