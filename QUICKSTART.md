# 0x3 ESP8266 Advanced Project - Quick Start Guide

This guide helps you quickly get started with the 0x3 Advanced ESP8266 Project.

## 1. Hardware Requirements

- ESP8266-based board (recommended: Wemos D1 Mini)
- Micro USB cable
- Computer with PlatformIO installed

## 2. Setup

### Option 1: Automated Setup (Recommended)

Run the setup script and follow the prompts:

```bash
./setup.sh
```

This will configure your project with your preferences.

### Option 2: Manual Setup

1. Edit `include/config.h` with your project details
2. Update `platformio.ini` with your board type if needed

## 3. Build & Upload

```bash
# Build the firmware
pio run

# Upload via USB (first time)
pio run --target upload

# Monitor serial output
pio device monitor
```

## 4. Initial Configuration

1. When first powered on, the device creates a WiFi access point named `0x3-esp_XXXXXX`
2. Connect to this access point using password from config.h (default: `0x3Config`)
3. A configuration portal should open automatically (or navigate to `192.168.4.1`)
4. Enter your WiFi network credentials
5. The device will restart and connect to your network

## 5. Accessing the Web Interface

1. After connecting to your WiFi, check the serial monitor for the device's IP address
2. Open a web browser and navigate to that IP address
3. You should see the device's web interface

## 6. Available Features

- **Web Interface**: Control and monitor your device
- **RESTful API**: Programmatic access at `/api/v1/...`
- **OTA Updates**: Update firmware at `/update`
- **mDNS Access**: Find device at `http://0x3-esp8266.local` (if supported by your OS)

## 7. OTA Updates

### Via Web Interface
1. Navigate to `http://[device-ip]/update`
2. Use credentials from config.h (defaults: username `0x3`, password `0x3Update`)
3. Select and upload new firmware file

### Via PlatformIO
Update `platformio.ini`:

```ini
upload_protocol = espota
upload_port = [device-ip]
```

Then run:
```bash
pio run --target upload
```

## 8. Troubleshooting

- **Device not connecting**: Try the configuration portal again or check credentials
- **Cannot access web interface**: Verify IP address and WiFi connection
- **OTA update fails**: Ensure stable WiFi connection and sufficient memory
- **LED patterns**: Refer to documentation for status indication meanings

## 9. Next Steps

- Check the full documentation in `README.md`
- Explore customization options in `config.h`
- Review API endpoints for integration with other systems
- Join the community for support and sharing

## 10. Need Help?

Refer to the complete documentation or open an issue in the repository.

Happy hacking! 🚀
