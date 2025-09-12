#pragma once

#include <unity.h>
#include <Arduino.h>

// Common test utilities and mocks for ESP32 tests

// WiFi mocks
#ifndef NATIVE_TEST
#include <WiFi.h>
#endif

// Mock WiFi class for desktop testing
#ifdef NATIVE_TEST

class MockWiFiClass {
public:
    bool begin(const char* ssid, const char* password) {
        strcpy(this->_ssid, ssid);
        strcpy(this->_password, password);
        this->_connected = true;
        return true;
    }
    
    bool disconnect(bool wifioff = false) {
        this->_connected = false;
        return true;
    }
    
    const char* SSID() const {
        return this->_ssid;
    }
    
    int32_t RSSI() const {
        return -60; // Mock reasonable RSSI value
    }
    
    uint8_t* macAddress(uint8_t* mac) const {
        static uint8_t defaultMac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        memcpy(mac, defaultMac, 6);
        return mac;
    }
    
    IPAddress localIP() const {
        return IPAddress(192, 168, 1, 100); // Mock IP
    }
    
    bool isConnected() const {
        return this->_connected;
    }
    
private:
    char _ssid[33] = {0};
    char _password[65] = {0};
    bool _connected = false;
};

extern MockWiFiClass WiFi;

class IPAddress {
public:
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        _address[0] = a;
        _address[1] = b;
        _address[2] = c;
        _address[3] = d;
    }
    
    uint8_t operator[](int index) const {
        return _address[index];
    }
    
    String toString() const {
        char buffer[16];
        sprintf(buffer, "%d.%d.%d.%d", _address[0], _address[1], _address[2], _address[3]);
        return String(buffer);
    }
    
private:
    uint8_t _address[4];
};

#endif // NATIVE_TEST

// Additional test helpers
namespace TestHelpers {
    // Function to generate random test data
    void generateRandomBytes(uint8_t* buffer, size_t len) {
        for (size_t i = 0; i < len; i++) {
            buffer[i] = (uint8_t)random(256);
        }
    }
    
    // Function to reset system state for tests
    void resetTestState() {
        #ifndef NATIVE_TEST
        delay(10); // Small delay to let any hardware actions complete
        #endif
    }
}
