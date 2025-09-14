// TFT_eSPI Configuration for ESP32 ADA Hardware
// This file should be placed in the TFT_eSPI library folder or use build flags

#ifndef _TFT_eSPIH_
#define _TFT_eSPIH_

// Driver selection
#define ILI9341_DRIVER      // Generic 320x240 TFT display driver
// #define ST7735_DRIVER    // Alternative for 128x160 displays
// #define ILI9488_DRIVER   // Alternative for 480x320 displays

// Display size
#define TFT_WIDTH  320
#define TFT_HEIGHT 240

// Pin configuration for ESP32 ADA board
#define TFT_MISO 19  // Shared with SD card
#define TFT_MOSI 23  // Shared with SD card  
#define TFT_SCLK 18  // Shared with SD card
#define TFT_CS   -1  // Not connected (always selected)
#define TFT_DC    2  // Data/Command pin
#define TFT_RST  -1  // Reset pin (not connected or tied to ESP32 reset)

// Backlight control (if available)
// #define TFT_BL   -1  // LED back-light control pin

// SPI frequency
#define SPI_FREQUENCY  40000000  // 40MHz for fast updates
#define SPI_READ_FREQUENCY  20000000

// Touch screen pins (if available)
// #define TOUCH_CS -1

// Font selection
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel high font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel high font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel high font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// FSPI port (default HSPI port)
#define SPI_PORT 1  // Set to 1 for FSPI port on ESP32

#endif // _TFT_eSPIH_