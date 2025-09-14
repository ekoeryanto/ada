# COMPLETE API ENDPOINT LIST - ADA Board
Based on thorough code analysis of `src/web_server.cpp`

✅ **FIXED**: Analog Current (4-20mA) endpoints now implemented!

## 🔧 System Management
- `GET /api/status` - System status and info
- `GET /api/health` - System health monitoring 
- `GET /api/config` - Configuration settings
- `POST /api/restart` - Restart the system
- `POST /api/reset` - Reset the system

## 📊 Analog Voltage / Sensors (0-10V)
- `GET /api/analog-voltage` - All sensor readings
- `GET /api/analog-voltage/health` - Sensor health status
- `GET /api/analog-voltage/info` - Sensor information
- `POST /api/analog-voltage/calibrate` - Calibrate sensors
- `POST /api/analog-voltage/reset-calibration` - Reset calibration

## ⚡ Analog Current (4-20mA) - ✅ NEW!
- `GET /api/analog-current` - All 4-20mA sensor readings
- `GET /api/analog-current/health` - Current loop health status  
- `GET /api/analog-current/info` - Sensor info & manufacturer details
- `GET /api/analog-current/diagnostics` - Loop diagnostics & resistance monitoring
- Configured sensors: Level Tank 2 (Rosemount), Flow Line 2 (Yokogawa), Pressure Sys (E+H)

## 🔌 Digital I/O - ✅ NEW!
- `GET /api/digital-io` - Complete digital I/O status (inputs + outputs)
- `GET /api/digital-io/inputs` - Digital input readings (DI1-DI4)
- `GET /api/digital-io/outputs` - Digital output status (DO1-DO4)
- `POST /api/digital-io/output` - Control outputs (ON/OFF/PULSE/BLINK)
- Pin support: 4 digital inputs with debounce, 4 outputs with PWM capability

## 💾 SD Card Management
- `GET /api/sd/status` - SD card status
- `POST /api/sd/test` - Test SD card functionality  
- `GET /api/sd/files` - List files on SD card

## 🎮 Simulation Mode
- `GET /api/simulation/status` - Simulation status
- `POST /api/simulation/enable` - Enable/disable simulation
- `POST /api/simulation/sensor` - Configure sensor simulation

## 📈 Analytics
- `GET /api/analytics/summary` - Analytics summary
- `GET /api/analytics/statistics` - Analytics statistics
- `GET /api/analytics/trends` - Data trends analysis
- `GET /api/analytics/prediction` - Predictive analytics

## 🔧 Diagnostics
- `GET /api/diagnostics/status` - Diagnostic status
- `GET /api/diagnostics/all` - All diagnostic data
- `GET /api/diagnostics/alerts` - Active alerts
- `GET /api/diagnostics/health` - Health diagnostics
- `POST /api/diagnostics/clear-alerts` - Clear alerts

## 🪝 Webhooks
- `GET /api/webhooks` - List webhooks
- `POST /api/webhooks` - Create webhook
- `GET /api/webhooks/status` - Webhook status
- `GET /api/webhooks/statistics` - Webhook statistics
- `GET /api/webhooks/queue` - Webhook queue status
- `POST /api/webhooks/queue/clear` - Clear webhook queue
- `POST /api/webhooks/queue/retry` - Retry failed webhooks
- `POST /api/webhooks/test` - Test webhook

## 🔌 Modbus Communication
- `GET /api/modbus/devices` - List Modbus devices
- `POST /api/modbus/devices` - Add Modbus device
- `GET /api/modbus/devices/*` - Get specific device info
- `DELETE /api/modbus/devices/*` - Remove device
- `POST /api/modbus/read` - Read from Modbus device
- `POST /api/modbus/write` - Write to Modbus device
- `POST /api/modbus/discover` - Discover Modbus devices
- `GET /api/modbus/stats` - Modbus statistics

## Summary
- **Total Implemented**: 34+ API endpoints
- **✅ COMPLETE**: All major functionality now has API access
- **🚀 INDUSTRIAL READY**: Both 0-10V and 4-20mA sensor endpoints available