#ifndef CONFIG_H
#define CONFIG_H

// 0x3 ESP32 Project Configuration
// Version: 1.0.0
// Author: 0x3

// Project Information
#define PROJECT_NAME "0x3-ESP32"
#define PROJECT_VERSION "1.0.0"
#define PROJECT_AUTHOR "0x3"

// WiFi Configuration
#define WIFI_TIMEOUT 30000          // 30 seconds
#define WIFI_RETRY_DELAY 5000       // 5 seconds
#define AP_PASSWORD "0x3Config"     // Default AP password
#define CONFIG_PORTAL_TIMEOUT 300   // 5 minutes

// OTA Configuration
#define OTA_USERNAME "0x3"
#define OTA_PASSWORD "0x3Update"
#define OTA_PORT 8080

// Web Server Configuration
#define WEB_SERVER_PORT 80

// Hardware Configuration - ESP32 Specific
// ESP32 built-in LED pins vary by board
#ifndef LED_BUILTIN
    #if defined(ARDUINO_ESP32_DEV) || defined(ARDUINO_ESP32_DOIT_DEVKIT_V1)
        #define LED_BUILTIN 2       // ESP32 DevKit V1
    #elif defined(ARDUINO_NODEMCU_32S)
        #define LED_BUILTIN 2       // NodeMCU-32S
    #elif defined(ARDUINO_LOLIN_D32)
        #define LED_BUILTIN 5       // LOLIN D32
    #elif defined(ARDUINO_LOLIN_D32_PRO)
        #define LED_BUILTIN 5       // LOLIN D32 Pro
    #else
        #define LED_BUILTIN 2       // Default for most ESP32 boards
    #endif
#endif

#define STATUS_LED_PIN LED_BUILTIN

// Debug Configuration
#define DEBUG_ENABLED true
#define DEBUG_BAUD_RATE 115200

// Network Configuration
#define HOSTNAME "0x3-esp32"

// Timing Configuration
#define HEARTBEAT_INTERVAL 30000    // 30 seconds
#define STATUS_UPDATE_INTERVAL 5000 // 5 seconds

#endif // CONFIG_H
