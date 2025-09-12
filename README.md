# 0x3 ESP32 Universal Template

A professional, modern ESP32 project template featuring WiFi Manager, OTA updates via ElegantOTA in async mode, and a beautiful web interface. Designed to be easily adapted for any ESP32 board and project.

## 🚀 Features

### Template Features
- **Board-Agnostic Design**: Works with any ESP32 board (ESP32, ESP32-S2, ESP32-S3, ESP32-C3)
- **Easy Customization**: Simple configuration via `config.h`
- **Automated Setup**: All necessary configurations in one place
- **Professional Structure**: Clean, modular, and maintainable code
- **Complete Documentation**: Comprehensive guides and examples

### Core Features
- **WiFi Manager**: Easy WiFi configuration without hardcoding credentials
- **Async Web Server**: Using ESPAsyncWebServer for responsive interface
- **OTA Updates**: Over-the-air firmware updates via ElegantOTA in async mode
- **Web Control Panel**: Beautiful, responsive web interface for monitoring and control
- **RESTful API**: JSON API endpoints for system information and control
- **Status LED**: Visual indication of system status via built-in LED
- **Modular Architecture**: Clean, maintainable code structure
- **Professional Branding**: Consistent 0x3 branding throughout

## 📱 Web Interface

The device provides a modern, responsive web interface accessible at the device's IP address. Features include:

- Real-time system status monitoring
- WiFi connection information
- OTA update interface
- Device restart and WiFi reset functionality
- Mobile-friendly responsive design
- Auto-refreshing data

## 🔧 Hardware Requirements

**Supported ESP32 Boards:**
- **ESP32 DevKit V1** (default, most common)
- **Generic ESP32**
- **NodeMCU-32S**
- **Wemos LOLIN D32/D32 Pro**
- **ESP32-S2**
- **ESP32-S3**
- **ESP32-C3**
- Any ESP32-based board

**Power Requirements:**
- 5V via USB or 3.3V direct
- Built-in LED used for status indication

**Board Selection:**
The template is designed to work with any ESP32 board. Simply update the `board` setting in `platformio.ini` and adjust the LED pin in `config.h` if needed.

## 📦 Dependencies

The project uses the following libraries (automatically installed by PlatformIO):

- `tzapu/WiFiManager` - WiFi configuration management
- `bblanchon/ArduinoJson` - JSON handling for API
- `AsyncTCP` - Async TCP support for ESP32
- `ESPAsyncWebServer` - Async web server
- `ayushsharma82/ElegantOTA` - OTA update functionality (in async mode)
- `Update` - Core ESP32 update library
- `Ticker` - Timer functionality

## � Quick Start with Template

### Option 1: Automated Setup (Recommended)
```bash
# Run the setup script for guided configuration
./setup.sh
```

### Option 2: Manual Setup
1. Copy this template to your project directory
2. Customize `include/config.h` with your project details
3. Select your board in `platformio.ini`
4. Build and upload

### Option 3: Use Template File
1. Copy `include/config_template.h` to `include/config.h`
2. Edit the copied file with your settings
3. Follow the instructions in the template file

See `TEMPLATE_USAGE.md` for detailed instructions.

### 2. Build and Upload

Using PlatformIO:

```bash
# Install dependencies and build
pio run

# Upload to device (first time via USB)
pio run --target upload

# Monitor serial output
pio device monitor
```

### 3. Initial Configuration

1. **First Boot**: Device will create a WiFi access point named `0x3-esp_[ChipID]`
2. **Connect to AP**: Use password `0x3Config`
3. **Configure WiFi**: Follow the captive portal to set up your WiFi credentials
4. **Access Web Interface**: Once connected, access the web interface at the device's IP address

## 🔄 OTA Updates

After initial setup, you can upload firmware updates over-the-air:

1. Access the web interface at the device's IP address
2. Click on "OTA Update" or navigate to `/update`
3. Use username: `0x3` and password: `0x3Update`
4. Select your firmware file (.bin) and click upload

Alternatively, configure PlatformIO for OTA updates:

```ini
# In platformio.ini, uncomment and update:
upload_protocol = espota
upload_port = 192.168.1.100  # Your device IP
upload_flags = 
    --auth=0x3Update
```

## 🌐 API Endpoints

The device provides several RESTful API endpoints:

### GET `/api/status`
Returns comprehensive system status in JSON format:

```json
{
  "project": "0x3-ESP",
  "version": "1.0.0",
  "author": "0x3",
  "status": "Running",
  "uptime": "1h 23m 45s",
  "freeHeap": 45312,
  "chipId": 12345678,
  "wifi": {
    "connected": true,
    "ssid": "MyNetwork",
    "ip": "192.168.1.100",
    "rssi": -45
  },
  "ota": {
    "enabled": true,
    "status": "Ready",
    "url": "http://192.168.1.100/update"
  }
}
```

### GET `/api/config`
Returns device configuration information

### POST `/api/restart`
Restarts the device

### POST `/api/reset`
Resets WiFi settings and restarts

## 🔄 OTA Updates

### Web Interface Method
1. Navigate to `http://[device-ip]/update`
2. Login with credentials:
   - **Username**: `0x3`
   - **Password**: `0x3Update`
3. Select firmware file (.bin)
4. Upload and wait for completion

### PlatformIO Method
After initial setup, you can upload via OTA:

```bash
# Update platformio.ini with device IP
# Then upload via OTA
pio run --target upload
```

## 💡 Status LED Patterns

The built-in LED indicates system status:

- **Fast Blink (200ms)**: Initializing
- **Medium Blink (500ms)**: Connecting to WiFi
- **Solid On**: Connected and running
- **Very Fast Blink (100ms)**: Error or WiFi failed
- **Ultra Fast Blink (50ms)**: OTA update in progress

## 🏗️ Project Structure

```
├── include/                  # Header files
│   ├── config.h             # Configuration constants (customize this)
│   ├── system_manager.h     # System management
│   ├── wifi_manager.h       # WiFi management
│   ├── web_server.h         # Web server handling
│   └── ota_handler.h        # OTA update handling
├── src/                     # Source files
│   ├── main.cpp            # Main application
│   ├── system_manager.cpp  # System management implementation
│   ├── wifi_manager.cpp    # WiFi management implementation
│   ├── web_server.cpp      # Web server implementation
│   └── ota_handler.cpp     # OTA handling implementation
├── tools/                   # Development tools
│   ├── esp32_devtools.py  # Unified developer tools interface
│   ├── memory_analyzer.py  # Memory usage analysis
│   ├── spiffs_uploader.py  # SPIFFS filesystem management
│   ├── config_generator.py # Configuration generator
│   ├── serial_monitor_plus.py # Enhanced serial monitor
│   └── wifi_scanner.py     # WiFi network scanner
├── lib/                    # Custom libraries (if any)
├── test/                   # Unit tests
├── README.md               # This file
└── platformio.ini         # PlatformIO configuration
```

## ⚙️ Configuration

Main configuration options in `include/config.h`:

```cpp
#define PROJECT_NAME "0x3-ESP"
#define PROJECT_VERSION "1.0.0"
#define PROJECT_AUTHOR "0x3"

#define WIFI_TIMEOUT 30000
#define AP_PASSWORD "0x3Config"
#define OTA_USERNAME "0x3"
#define OTA_PASSWORD "0x3Update"
#define WEB_SERVER_PORT 80
```

## 🔧 Customization

### Adding Custom Features

1. **Custom Web Routes**: Use `webServer.addCustomRoute()` in main.cpp
2. **Custom Status**: Extend `SystemStatus` enum in system_manager.h
3. **Custom Configuration**: Add parameters to WiFiManager in wifi_manager.cpp
4. **Custom API Endpoints**: Add routes in web_server.cpp

### Branding Customization

1. Update constants in `config.h`
2. Modify web interface styling in `web_server.cpp`
3. Update welcome banner in `system_manager.cpp`

## 🔧 Development Tools

This project includes a suite of specialized development tools in the `tools/` directory to enhance ESP32 development:

### Memory Analyzer
Analyze and optimize memory usage in your firmware:
```bash
./tools/esp32_devtools.py memory --elf .pio/build/esp32doit-devkit-v1/firmware.elf
```

### SPIFFS File Manager
Build and upload web content to the ESP32 filesystem:
```bash
./tools/esp32_devtools.py spiffs --data ./data --upload
```

### Configuration Generator
Interactive configuration tool to generate config.h:
```bash
./tools/esp32_devtools.py config --interactive
```

### Enhanced Serial Monitor
Serial monitor with JSON parsing and data export:
```bash
./tools/esp32_devtools.py monitor --json --log output.txt
```

### WiFi Scanner
Scan and analyze available WiFi networks:
```bash
./tools/esp32_devtools.py wifi --export wifi_scan.csv
```

See `tools/README.md` for detailed documentation of each tool.

## 🐛 Troubleshooting

### WiFi Connection Issues
- Check AP password (default: `0x3Config`)
- Verify network credentials
- Check signal strength
- Reset WiFi settings via web interface

### OTA Update Issues
- Ensure stable WiFi connection
- Check firmware file compatibility
- Verify OTA credentials
- Monitor serial output for errors

### Memory Issues
- Monitor free heap via web interface
- Check for memory leaks in custom code
- Reduce string usage if needed

## 📈 Monitoring

The system provides comprehensive monitoring:

- **Serial Output**: Detailed logging and debug information
- **Web Interface**: Real-time status updates
- **LED Status**: Visual system state indication
- **API Endpoints**: Programmatic access to system data
- **Heartbeat**: Periodic status reports

## 🔒 Security Notes

- Change default passwords in production
- Use HTTPS in production environments
- Consider implementing authentication for API endpoints
- Regular firmware updates recommended

## � License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## � Author

Created by 0x3 (http://github.com/0x3)

---

Made with ❤️ by 0x3 - Professional ESP32 Framework

## 🧪 Testing

This project includes comprehensive testing capabilities:

### Unit Testing

The project uses Unity test framework and PlatformIO's built-in test runner:

```bash
# Run all tests
platformio test

# Run native tests only (no hardware required)
platformio test -e test_desktop

# Run complete test suite with report generation
./tools/run_tests.sh
```

### Continuous Integration

GitHub Actions CI is configured to:
- Build the project for all supported ESP32 boards
- Run unit tests in desktop environment
- Run static code analysis
- Generate test reports

CI builds are triggered on:
- Every push to main/master/develop branches
- Every pull request to these branches
- Manual trigger via GitHub Actions UI

### Test Structure

- `test/test_wifi_manager/` - WiFi Manager unit tests
- `test/test_system_manager/` - System Manager unit tests
- `test/test_integration/` - Integration tests across components
- `test/test_utils.h` - Shared test utilities and mocks

For more details, see the test files and CI configuration in `.github/workflows/ci.yml`.
