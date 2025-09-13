#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

// Digital Inputs (3.3V - 24V compatible)
#define DI1_PIN 27
#define DI2_PIN 26
#define DI3_PIN 25
#define DI4_PIN 33

// Analog Inputs (0-10V)
#define AI1_PIN 35
#define AI2_PIN 34
#define AI3_PIN 36

// Digital Outputs
#define DO1_PIN 15
#define DO2_PIN 13
#define DO3_PIN 12
#define DO4_PIN 14

// RS485 pins
#define RS485_RX 16
#define RS485_TX 17
#define RS485_DE 4

// SD Card pins (SPI)
#define SD_CS 5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

// I2C pins for ADS1115 and RTC
#define I2C_SDA 21
#define I2C_SCL 22

// ADS1115 Address
#define ADS1115_ADDR 0x48

#endif // PINS_CONFIG_H
