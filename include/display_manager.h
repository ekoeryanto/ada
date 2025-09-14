#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>

// Display configuration
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_ROTATION 1

// Colors (16-bit RGB565)
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFD20
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_GRAY      0x8410
#define COLOR_DARKGRAY  0x4208
#define COLOR_LIGHTGRAY 0xC618

// Display pages
enum DisplayPage {
    PAGE_SPLASH = 0,
    PAGE_STATUS,
    PAGE_ANALOG_VOLTAGE,
    PAGE_ANALOG_CURRENT,
    PAGE_DIGITAL_IO,
    PAGE_MODBUS,
    PAGE_NETWORK,
    PAGE_ALARMS,
    PAGE_SETTINGS,
    PAGE_COUNT
};

// Display update intervals (ms)
#define DISPLAY_UPDATE_FAST     100   // For critical data
#define DISPLAY_UPDATE_NORMAL   500   // For regular data
#define DISPLAY_UPDATE_SLOW     2000  // For status info

// Button configuration (if hardware buttons exist)
#define BUTTON_NEXT_PIN     -1    // Configure if buttons exist
#define BUTTON_SELECT_PIN   -1    // Configure if buttons exist
#define BUTTON_BACK_PIN     -1    // Configure if buttons exist

struct DisplayData {
    // System status
    String deviceName;
    String wifiSSID;
    int wifiRSSI;
    String ipAddress;
    unsigned long uptime;
    float freeHeap;
    
    // Sensor data
    float analogVoltage[3];
    float analogCurrent[3];
    String analogVoltageStatus[3];
    String analogCurrentStatus[3];
    
    // Digital I/O
    bool digitalInputs[4];
    bool digitalOutputs[4];
    String digitalInputNames[4];
    String digitalOutputNames[4];
    
    // Alarms
    int activeAlarms;
    String lastAlarmMessage;
    
    // Modbus
    int modbusDevices;
    String modbusStatus;
    
    // Timestamps
    unsigned long lastUpdate;
    bool dataValid;
};

class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();
    
    // Core functions
    bool begin();
    void update();
    void loop();
    
    // Display control
    void setPage(DisplayPage page);
    DisplayPage getCurrentPage();
    void nextPage();
    void previousPage();
    void refresh();
    void clearScreen();
    void setBrightness(uint8_t brightness);
    
    // Data update functions
    void updateSystemData(const JsonObject& data);
    void updateAnalogVoltageData(const JsonObject& data);
    void updateAnalogCurrentData(const JsonObject& data);
    void updateDigitalIOData(const JsonObject& data);
    void updateModbusData(const JsonObject& data);
    void updateAlarmData(int count, const String& message);
    
    // Status functions
    bool isInitialized() const { return initialized; }
    void showError(const String& message);
    void showMessage(const String& message, uint16_t color = COLOR_WHITE);
    
    // Button handling (if hardware buttons exist)
    void handleButtons();
    
private:
    TFT_eSPI tft;
    DisplayData data;
    DisplayPage currentPage;
    unsigned long lastUpdate;
    unsigned long lastPageChange;
    bool initialized;
    uint8_t brightness;
    bool autoRotate;
    unsigned long autoRotateInterval;
    
    // Display functions
    void drawSplashScreen();
    void drawStatusPage();
    void drawAnalogVoltagePage();
    void drawAnalogCurrentPage();
    void drawDigitalIOPage();
    void drawModbusPage();
    void drawNetworkPage();
    void drawAlarmsPage();
    void drawSettingsPage();
    
    // Helper functions
    void drawHeader(const String& title);
    void drawFooter();
    void drawStatusBar();
    void drawDataValue(int x, int y, const String& label, const String& value, uint16_t color = COLOR_WHITE);
    void drawProgressBar(int x, int y, int width, int height, float percentage, uint16_t color);
    void drawAlarmIndicator(int x, int y, bool active);
    void drawConnectionStatus(int x, int y, bool connected);
    void drawSensorStatus(int x, int y, const String& status);
    
    // Data formatting
    String formatUptime(unsigned long uptimeMs);
    String formatMemory(float bytes);
    String formatRSSI(int rssi);
    uint16_t getStatusColor(const String& status);
    
    // Auto-rotation
    void checkAutoRotation();
    
    // Error handling
    void handleDisplayError(const String& error);
};

// Global instance
extern DisplayManager displayMgr;

#endif // DISPLAY_MANAGER_H