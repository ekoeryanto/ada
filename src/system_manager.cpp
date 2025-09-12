#include "system_manager.h"
#include "wifi_manager.h"
#include "ota_handler.h"

// Global instance
SystemManager systemMgr;

SystemManager::SystemManager() {
    currentStatus = SYSTEM_INITIALIZING;
    uptimeStart = 0;
    debugEnabled = DEBUG_ENABLED;
}

bool SystemManager::initialize() {
    Serial.begin(DEBUG_BAUD_RATE);
    delay(100);
    
    // Print welcome banner
    printWelcomeBanner();
    
    // Initialize built-in LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    setStatusLED(HIGH); // Turn off LED initially (inverted logic)
    
    // Start uptime counter
    uptimeStart = millis();
    
    // Setup heartbeat
    heartbeatTicker.attach(HEARTBEAT_INTERVAL / 1000, heartbeatCallback);
    
    Serial.println("[System] System manager initialized");
    printSystemInfo();
    
    return true;
}

void SystemManager::loop() {
    // Update status LED based on current status
    updateStatusLED();
}

void SystemManager::setStatus(SystemStatus status) {
    if (currentStatus != status) {
        currentStatus = status;
        Serial.printf("[System] Status changed to: %s\n", getStatusString().c_str());
        updateStatusLED();
    }
}

SystemStatus SystemManager::getStatus() {
    return currentStatus;
}

String SystemManager::getStatusString() {
    switch (currentStatus) {
        case SYSTEM_INITIALIZING:   return "Initializing";
        case SYSTEM_WIFI_CONNECTING: return "WiFi Connecting";
        case SYSTEM_WIFI_CONNECTED: return "WiFi Connected";
        case SYSTEM_WIFI_FAILED:    return "WiFi Failed";
        case SYSTEM_RUNNING:        return "Running";
        case SYSTEM_OTA_UPDATE:     return "OTA Update";
        case SYSTEM_ERROR:          return "Error";
        default:                    return "Unknown";
    }
}

unsigned long SystemManager::getUptime() {
    return (millis() - uptimeStart) / 1000;
}

String SystemManager::getUptimeString() {
    unsigned long uptime = getUptime();
    unsigned long days = uptime / 86400;
    unsigned long hours = (uptime % 86400) / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;
    
    String result = "";
    if (days > 0) {
        result += String(days) + "d ";
    }
    if (hours > 0 || days > 0) {
        result += String(hours) + "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        result += String(minutes) + "m ";
    }
    result += String(seconds) + "s";
    
    return result;
}

void SystemManager::restart() {
    Serial.println("[System] Restarting system...");
    delay(1000);
    ESP.restart();
}

void SystemManager::factoryReset() {
    Serial.println("[System] Performing factory reset...");
    
    // Reset WiFi settings
    wifiMgr.resetAllSettings();
    
    delay(2000);
    restart();
}

void SystemManager::enableDebug(bool enable) {
    debugEnabled = enable;
    Serial.printf("[System] Debug %s\n", enable ? "enabled" : "disabled");
}

uint32_t SystemManager::getFreeHeap() {
    return ESP.getFreeHeap();
}

uint8_t SystemManager::getHeapFragmentation() {
    // ESP32 doesn't have getHeapFragmentation, return 0
    return 0;
}

uint64_t SystemManager::getChipId() {
    return ESP.getEfuseMac();
}

String SystemManager::getChipInfo() {
    String info = "";
    info += "Chip ID: 0x" + String(getChipId(), HEX) + "\n";
    
    // ESP32-specific chip information
    info += "Chip Model: " + String(ESP.getChipModel()) + "\n";
    info += "Chip Revision: " + String(ESP.getChipRevision()) + "\n";
    info += "Flash Size: " + String(ESP.getFlashChipSize()) + " bytes\n";
    info += "Flash Speed: " + String(ESP.getFlashChipSpeed()) + " Hz\n";
    info += "CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    info += "SDK Version: " + String(ESP.getSdkVersion()) + "\n";
    
    return info;
}

String SystemManager::getSystemInfo() {
    String info = "";
    info += "=== " + String(PROJECT_NAME) + " v" + String(PROJECT_VERSION) + " ===\n";
    info += "Author: " + String(PROJECT_AUTHOR) + "\n";
    info += "Status: " + getStatusString() + "\n";
    info += "Uptime: " + getUptimeString() + "\n";
    info += "Free Heap: " + String(getFreeHeap()) + " bytes\n";
    info += "Heap Fragmentation: " + String(getHeapFragmentation()) + "%\n";
    info += getChipInfo();
    
    return info;
}

void SystemManager::printSystemInfo() {
    Serial.println("\n" + getSystemInfo());
}

void SystemManager::printWelcomeBanner() {
    Serial.println();
    Serial.println("╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║                                                              ║");
    Serial.printf("║                        %s v%s                         ║\n", PROJECT_NAME, PROJECT_VERSION);
    Serial.println("║                                                              ║");
    Serial.printf("║                      Created by %s                         ║\n", PROJECT_AUTHOR);
    Serial.println("║                                                              ║");
    Serial.println("║              ESP32 with WiFi Manager & OTA                 ║");
    Serial.println("║                                                              ║");
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
    Serial.println();
}

void SystemManager::updateStatusLED() {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    
    if (now - lastUpdate < 100) return; // Update at most every 100ms
    lastUpdate = now;
    
    switch (currentStatus) {
        case SYSTEM_INITIALIZING:
            blinkStatusLED(200); // Fast blink
            break;
        case SYSTEM_WIFI_CONNECTING:
            blinkStatusLED(500); // Medium blink
            break;
        case SYSTEM_WIFI_CONNECTED:
        case SYSTEM_RUNNING:
            setStatusLED(LOW); // Solid on
            break;
        case SYSTEM_WIFI_FAILED:
        case SYSTEM_ERROR:
            blinkStatusLED(100); // Very fast blink
            break;
        case SYSTEM_OTA_UPDATE:
            blinkStatusLED(50); // Ultra fast blink
            break;
        default:
            setStatusLED(HIGH); // Off
            break;
    }
}

void SystemManager::setStatusLED(bool state) {
    statusLedTicker.detach();
    digitalWrite(STATUS_LED_PIN, state ? LOW : HIGH); // Inverted logic for built-in LED
}

void SystemManager::blinkStatusLED(int interval) {
    static int lastInterval = -1;
    
    if (lastInterval != interval) {
        statusLedTicker.detach();
        statusLedTicker.attach_ms(interval, toggleStatusLED);
        lastInterval = interval;
    }
}

void SystemManager::toggleStatusLED() {
    static bool ledState = false;
    ledState = !ledState;
    digitalWrite(STATUS_LED_PIN, ledState ? LOW : HIGH); // Inverted logic
}

void SystemManager::heartbeatCallback() {
    if (systemMgr.debugEnabled && systemMgr.currentStatus == SYSTEM_RUNNING) {
        Serial.printf("[System] Heartbeat - Uptime: %s, Free Heap: %u bytes\n", 
                     systemMgr.getUptimeString().c_str(), 
                     systemMgr.getFreeHeap());
    }
}
