# 0x3 ESP Project Changelog

All notable changes to this project will be documented in this file.

## [2.0.0] - 2025-06-17

### Added
- Advanced mDNS support for local network service discovery
- Versioned API endpoints with `/api/v1/` structure
- Enhanced telemetry and monitoring capabilities
- JSON-structured logging for better debugging
- Memory utilization monitoring and reporting
- Extended WiFi stability improvements
- Modern ESP8266WebServer implementation replacing AsyncWebServer
- Improved modularity and code organization
- Expanded system information endpoints
- Heartbeat system for monitoring device health

### Changed
- Upgraded to modern ESP8266 libraries
- Replaced AsyncWebServer with ESP8266WebServer for better reliability
- Optimized memory usage for better performance
- Updated OTA process with improved error handling
- Enhanced status LED patterns for better visual feedback
- Updated PlatformIO configuration for modern development
- Improved documentation with detailed API reference

### Fixed
- Memory leak in web server request handling
- WiFi reconnection issues after network interruptions
- LED status inconsistencies in certain states
- OTA update process occasionally failing to complete
- Configuration portal timeout handling

## [1.0.0] - 2025-06-16

### Added
- Initial release of 0x3 ESP Project
- WiFi Manager integration for easy network configuration
- Over-The-Air (OTA) update capability via web interface
- Beautiful responsive web control panel
- RESTful API endpoints for system monitoring
- Modular code architecture with clean separation of concerns
- Professional 0x3 branding throughout
- Status LED indication for system states
- Comprehensive error handling and recovery
- Factory reset and WiFi reset functionality
- Real-time system monitoring (uptime, memory, network status)
- Auto-reconnection handling for WiFi
- Heartbeat monitoring with periodic status reports
- Debug logging with configurable levels
- Mobile-friendly responsive web design
- Secure OTA updates with authentication
- System information API endpoints
- Configuration portal with timeout handling
- Built-in LED status indication patterns
- Memory usage monitoring and reporting
