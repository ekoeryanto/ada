# 0x3 Advanced ESP8266 Project

A professional, modern ESP8266 project featuring advanced capabilities including WiFi Manager, versioned API endpoints, mDNS service discovery, OTA updates, and a beautiful web interface. This project represents an evolution of the basic ESP8266 template with enhanced features and optimizations.

## 🚀 Features

### Advanced Features
- **mDNS Support**: Local network service discovery
- **Versioned API**: RESTful API with versioned endpoints (/api/v1/*)
- **Enhanced Telemetry**: Comprehensive system metrics and diagnostics
- **JSON Logging**: Structured logging for better diagnostics
- **Optimized Memory Usage**: Balanced for both features and performance
- **Advanced Error Handling**: Improved stability and error recovery

### Core Features
- **WiFi Manager**: Easy WiFi configuration without hardcoding credentials
- **OTA Updates**: Over-the-air firmware updates via web interface
- **Web Control Panel**: Beautiful, responsive web interface for monitoring and control
- **RESTful API**: Comprehensive JSON API endpoints for system information and control
- **Status LED**: Visual indication of system status with enhanced states
- **Modular Architecture**: Clean, maintainable code structure with improved organization
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

**Supported ESP8266 Boards:**
- **Wemos D1 Mini** (default, recommended)
- **NodeMCU v2** 
- **ESP-12E/ESP-12F**
- **ESP-01** (limited features due to memory constraints)
- **ESP-07**
- Any ESP8266-based board

**Power Requirements:**
- 5V via USB or 3.3V direct
- Built-in LED used for status indication

**Board Selection:**
The template is designed to work with any ESP8266 board. Simply update the `board` setting in `platformio.ini` and adjust the LED pin in `config.h` if needed.

## 📦 Dependencies

The project uses the following libraries (automatically installed by PlatformIO):

- `tzapu/WiFiManager` - WiFi configuration management
- `bblanchon/ArduinoJson` - JSON handling for API
- `ESP8266WebServer` - Built-in web server functionality
- `ESP8266HTTPUpdateServer` - Built-in OTA updates
- `ESP8266WiFi` - WiFi functionality
- `ESP8266mDNS` - mDNS for service discovery
- `Ticker` - System timing and callbacks

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
3. **Configure WiFi**: Browser will automatically open configuration page, or go to `192.168.4.1`
4. **Enter Credentials**: Provide your WiFi network credentials
5. **Save & Restart**: Device will restart and connect to your network

### 4. Access Web Interface

After successful WiFi connection:

1. Check serial monitor for IP address
2. Open web browser and navigate to the device IP
3. Enjoy the beautiful control panel!

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
├── include/                    # Header files
│   ├── config.h               # Configuration constants (customize this)
│   ├── config_template.h      # Template configuration file
│   ├── system_manager.h       # System management
│   ├── wifi_manager.h         # WiFi management
│   ├── web_server.h           # Web server handling
│   └── ota_handler.h          # OTA update handling
├── src/                       # Source files
│   ├── main.cpp              # Main application
│   ├── system_manager.cpp    # System management implementation
│   ├── wifi_manager.cpp      # WiFi management implementation
│   ├── web_server.cpp        # Web server implementation
│   └── ota_handler.cpp       # OTA handling implementation
├── lib/                      # Custom libraries (if any)
├── test/                     # Unit tests
├── setup.sh                  # Automated setup script
├── TEMPLATE_USAGE.md         # Template usage guide
├── README.md                 # This file
├── CHANGELOG.md              # Version history
└── platformio.ini           # PlatformIO configuration
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

## 📄 License

This project is created by 0x3. Feel free to use and modify according to your needs.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## 📞 Support

For support and questions, please open an issue in the project repository.

---

**0x3 ESP Project v1.0.0** - Professional ESP8266 Development Platform
