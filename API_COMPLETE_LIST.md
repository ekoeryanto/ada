# COMPLETE API ENDPOINT LIST - ADA Board
Based on thorough code analysis of `src/web_server.cpp`

⚠️ **CRITICAL ISSUE FOUND**: Analog Current (4-20mA) manager exists but has NO API endpoints!

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

## ❌ MISSING: Analog Current (4-20mA) 
**MAJOR BUG**: AnalogCurrentManager exists with 3 configured sensors but NO API endpoints!
- Missing: `GET /api/analog-current` 
- Missing: `GET /api/analog-current/health`
- Missing: `GET /api/analog-current/info`
- Missing: `POST /api/analog-current/calibrate`
- Configured sensors: Level Tank 2, Flow Line 2, Pressure Sys (Rosemount, Yokogawa, E+H)

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
- **Total Implemented**: 30+ API endpoints
- **Total Should Be**: 36+ endpoints (missing 6 analog-current endpoints)
- **Critical Issue**: Industrial 4-20mA sensors not accessible via API!