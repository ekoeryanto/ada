# 🚀 ESP32 Deployment Guide

## 📋 Environment Setup Summary

Proyek ini menggunakan **dual environment** untuk mendukung USB dan OTA upload:

### 🔌 USB Environment (`esp32doit-devkit-v1`)
- **Usage**: First-time flashing, development, debugging
- **Upload**: Via USB cable (esptool)
- **Command**: `pio run -t upload -e esp32doit-devkit-v1`

### 📡 OTA Environment (`ota`)
- **Usage**: Wireless firmware updates
- **Upload**: Via WiFi network (espota)
- **Command**: `pio run -t upload -e ota`
- **Requirements**: ESP32 must be connected to WiFi with known IP

## 🛠️ Deployment Commands

### Initial Setup (First Upload)
```bash
# 1. Connect ESP32 via USB cable
# 2. Upload firmware via USB
pio run -t upload -e esp32doit-devkit-v1

# 3. Monitor serial output to get IP address
pio device monitor
```

### OTA Updates (After Initial Setup)
```bash
# 1. Update IP address in platformio.ini if needed
# 2. Upload via OTA
pio run -t upload -e ota

# Alternative: Build only
pio run -e ota
```

### Build Only (No Upload)
```bash
# Build for USB environment
pio run -e esp32doit-devkit-v1

# Build for OTA environment  
pio run -e ota

# Build both environments
pio run
```

## 🔧 Configuration

### OTA Settings (in platformio.ini)
```ini
[env:ota]
upload_protocol = espota
upload_port = 192.168.111.34  # Update with actual ESP32 IP
upload_flags = 
    --auth=Mar9aMulya         # OTA password
    --port=3232               # OTA port
```

### Partition Scheme
- **Custom partition**: `partitions_ota.csv`
- **App0**: 1.5MB (primary firmware)
- **App1**: 1.5MB (OTA firmware)
- **SPIFFS**: 960KB (file storage)
- **NVS**: 20KB (settings storage)
- **OTA Data**: 8KB (OTA management)

## 📊 Memory Usage
- **Current firmware**: ~1.41MB (90.1% of 1.5MB partition)
- **RAM usage**: ~69KB (21.2% of 320KB)
- **Development margin**: ~156KB available

## 🔐 Security Features
- **OTA Authentication**: Password-protected updates
- **Persistent Storage**: Settings survive power cycles
- **WiFi Credentials**: Stored securely in NVS

## 🌐 Web Interface Access
1. **Development**: `http://localhost:4321` (Astro dev server)
2. **Production**: `http://[ESP32_IP]` (direct ESP32 access)
3. **OTA Updates**: `http://[ESP32_IP]/update` (ElegantOTA)

## 🔄 Persistent Storage
All configuration automatically persists across reboots:
- ✅ WiFi credentials & network settings
- ✅ NTP server & timezone configuration  
- ✅ Analog voltage calibration data
- ✅ Analog current calibration data
- ✅ Webhook configurations & metadata
- ✅ System preferences & settings

## 🆘 Troubleshooting

### OTA Upload Failed
```bash
# Check ESP32 connectivity
ping 192.168.111.34

# Verify OTA service is running
curl http://192.168.111.34/update

# Use USB fallback
pio run -t upload -e esp32doit-devkit-v1
```

### Build Errors
```bash
# Clean build cache
pio run -t clean

# Rebuild from scratch
pio run -e esp32doit-devkit-v1
```

### Memory Issues
- Current build uses 90.1% of partition
- If exceeding 100%, consider:
  - Removing unused libraries
  - Disabling debug flags
  - Using compiler optimizations

## 📝 Notes
- Always test USB upload first before enabling OTA
- OTA requires stable WiFi connection
- Backup firmware before major updates
- Monitor serial output during first boot for debugging

---
**Last Updated**: September 15, 2025  
**ESP32 Status**: ✅ Production Ready  
**OTA Support**: ✅ Enabled  
**Persistent Storage**: ✅ Complete