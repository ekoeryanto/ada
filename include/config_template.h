# 0x3 ESP8266 Template Configuration
# Copy this file to config_template.h and customize for your project

#ifndef CONFIG_TEMPLATE_H
#define CONFIG_TEMPLATE_H

// ============================================================================
// PROJECT INFORMATION - CUSTOMIZE THESE FOR YOUR PROJECT
// ============================================================================
#define PROJECT_NAME "YourProjectName"        // Change to your project name
#define PROJECT_VERSION "1.0.0"               // Your project version
#define PROJECT_AUTHOR "YourName"             // Your name or organization

// ============================================================================
// NETWORK CONFIGURATION - CUSTOMIZE FOR YOUR DEPLOYMENT
// ============================================================================
#define HOSTNAME "your-device"                // Device hostname (lowercase, no spaces)
#define AP_PASSWORD "YourConfigPassword"      // Password for configuration AP
#define CONFIG_PORTAL_TIMEOUT 300             // Configuration portal timeout (seconds)

// WiFi Connection Settings
#define WIFI_TIMEOUT 30000                    // WiFi connection timeout (ms)
#define WIFI_RETRY_DELAY 5000                 // Delay between reconnection attempts (ms)

// ============================================================================
// OTA (Over-The-Air) UPDATE CONFIGURATION
// ============================================================================
#define OTA_USERNAME "yourusername"           // OTA update username
#define OTA_PASSWORD "yourpassword"           // OTA update password
#define OTA_PORT 8080                         // OTA service port

// ============================================================================
// WEB SERVER CONFIGURATION
// ============================================================================
#define WEB_SERVER_PORT 80                    // Web server port

// ============================================================================
// HARDWARE CONFIGURATION - BOARD SPECIFIC
// ============================================================================
// LED Configuration (varies by board)
// Wemos D1 Mini: 2, NodeMCU: 2 or 16, ESP-12E: 2, ESP-01: 1
#define LED_BUILTIN 2                         // Built-in LED pin
#define STATUS_LED_PIN LED_BUILTIN            // Pin for status indication

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================
// Debug Settings
#define DEBUG_ENABLED true                    // Enable debug output
#define DEBUG_BAUD_RATE 115200                // Serial baud rate

// Timing Configuration
#define HEARTBEAT_INTERVAL 30000              // Heartbeat interval (ms)
#define STATUS_UPDATE_INTERVAL 5000           // Status update interval (ms)

// ============================================================================
// ADVANCED CONFIGURATION (Usually no need to change)
// ============================================================================

// Memory Management
#define MIN_FREE_HEAP 8192                    // Minimum free heap warning threshold

// Network Timeouts
#define HTTP_TIMEOUT 10000                    // HTTP request timeout
#define DNS_TIMEOUT 5000                      // DNS resolution timeout

// System Limits
#define MAX_CONFIG_SIZE 512                   // Maximum configuration size
#define MAX_LOG_ENTRIES 50                    // Maximum log entries to keep

// ============================================================================
// BOARD-SPECIFIC CONFIGURATIONS
// ============================================================================

#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
    // Wemos D1 Mini specific settings
    #define BOARD_NAME "Wemos D1 Mini"
    #define BOARD_LED_PIN 2
    #define BOARD_FLASH_SIZE "4MB"
#elif defined(ARDUINO_ESP8266_NODEMCU)
    // NodeMCU specific settings
    #define BOARD_NAME "NodeMCU"
    #define BOARD_LED_PIN 2
    #define BOARD_FLASH_SIZE "4MB"
#elif defined(ARDUINO_ESP8266_ESP12)
    // ESP-12E specific settings
    #define BOARD_NAME "ESP-12E"
    #define BOARD_LED_PIN 2
    #define BOARD_FLASH_SIZE "4MB"
#elif defined(ARDUINO_ESP8266_ESP01)
    // ESP-01 specific settings
    #define BOARD_NAME "ESP-01"
    #define BOARD_LED_PIN 1
    #define BOARD_FLASH_SIZE "1MB"
    // Reduce features for limited memory
    #undef HEARTBEAT_INTERVAL
    #define HEARTBEAT_INTERVAL 60000          // Longer interval for ESP-01
    #undef MAX_LOG_ENTRIES
    #define MAX_LOG_ENTRIES 10                // Fewer log entries
#else
    // Generic ESP8266 settings
    #define BOARD_NAME "ESP8266"
    #define BOARD_LED_PIN 2
    #define BOARD_FLASH_SIZE "Unknown"
#endif

// ============================================================================
// FEATURE FLAGS - ENABLE/DISABLE FEATURES BASED ON BOARD CAPABILITIES
// ============================================================================

// Enable features based on available flash memory
#if defined(ARDUINO_ESP8266_ESP01)
    // Limited features for ESP-01
    #define ENABLE_WEB_LOGGING false
    #define ENABLE_DETAILED_STATUS false
    #define ENABLE_FILE_SYSTEM false
#else
    // Full features for other boards
    #define ENABLE_WEB_LOGGING true
    #define ENABLE_DETAILED_STATUS true
    #define ENABLE_FILE_SYSTEM true
#endif

// ============================================================================
// VALIDATION AND SAFETY CHECKS
// ============================================================================

// Ensure required definitions exist
#ifndef PROJECT_NAME
    #error "PROJECT_NAME must be defined"
#endif

#ifndef HOSTNAME
    #error "HOSTNAME must be defined"
#endif

// Validate configuration values
#if WIFI_TIMEOUT < 5000
    #warning "WIFI_TIMEOUT is very short, consider increasing"
#endif

#if CONFIG_PORTAL_TIMEOUT < 60
    #warning "CONFIG_PORTAL_TIMEOUT is very short"
#endif

// ============================================================================
// COMPUTED VALUES (Do not modify)
// ============================================================================

// Generate unique AP name with chip ID
#define AP_NAME_PREFIX HOSTNAME
#define DEVICE_ID_STRING String(HOSTNAME) + "_" + String(ESP.getChipId(), HEX)

#endif // CONFIG_TEMPLATE_H

// ============================================================================
// USAGE INSTRUCTIONS
// ============================================================================
/*

To use this template:

1. Copy this file to your project and rename it to match your needs
2. Update all the "YOUR..." placeholders with your actual values
3. Choose appropriate board-specific settings
4. Enable/disable features based on your requirements
5. Test thoroughly on your target hardware

Example customization for a temperature sensor project:

#define PROJECT_NAME "TemperatureSensor"
#define PROJECT_VERSION "2.1.0"
#define PROJECT_AUTHOR "MyCompany"
#define HOSTNAME "temp-sensor"
#define AP_PASSWORD "TempConfig123"
#define OTA_USERNAME "admin"
#define OTA_PASSWORD "TempUpdate456"

Remember to:
- Use secure passwords in production
- Test WiFi configuration on your target network
- Verify OTA updates work correctly
- Check memory usage on resource-constrained boards

*/
