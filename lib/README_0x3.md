This directory is for custom libraries used by the 0x3 ESP Project.

Currently, the project uses external libraries managed by PlatformIO's
library manager, as defined in platformio.ini:

External Dependencies:
- tzapu/WiFiManager: WiFi configuration management
- bblanchon/ArduinoJson: JSON handling for API responses
- ayushsharma82/AsyncElegantOTA: OTA update functionality
- ottowinter/ESPAsyncWebServer-esphome: Async web server
- ottowinter/AsyncTCP-esphome: Async TCP support

If you need to add custom libraries specific to this project,
place them in this directory. Each library should have its own
subdirectory with the following structure:

lib/
├── your_library_name/
│   ├── library.json
│   ├── src/
│   │   ├── your_library.h
│   │   └── your_library.cpp
│   └── examples/

For more information about custom libraries in PlatformIO:
https://docs.platformio.org/en/latest/librarymanager/
