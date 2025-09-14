# ESP32 API Endpoints Reference

## Available Endpoints (from ESP32 web_server.cpp)

### System & Status
- `GET /api/status` - System status, uptime, memory, WiFi info
- `GET /api/config` - System configuration

### Analog Voltage (Sensors)
- `GET /api/analog-voltage` - Current sensor readings
# ESP32 Ada Sensor API Endpoints

## System Status
- `GET /api/status` - System status and configuration

## Analog Voltage Management
- `GET /api/analog-voltage` - Get analog voltage readings
- `GET /api/analog-voltage/health` - Get sensor health status
- `POST /api/analog-voltage/calibrate` - Calibrate sensors
- `POST /api/analog-voltage/reset-calibration` - Reset calibration
- `GET /api/analog-voltage/info` - Get sensor information

## Configuration
- `GET /api/config` - Get system configuration

## System Control
- `POST /api/restart` - Restart the ESP32 device
- `POST /api/reset` - Reset WiFi settings (device will restart)

## SD Card
- `GET /api/sd/status` - SD card status
- `POST /api/sd/test` - Test SD card functionality
- `GET /api/sd/files` - List files on SD card

## Simulation (Development)
- `POST /api/simulation/enable` - Enable/disable simulation mode
- `POST /api/simulation/sensor` - Configure simulated sensor data
- `GET /api/simulation/status` - Get simulation status

## Analytics
- `GET /api/analytics/summary` - Analytics summary
- `GET /api/analytics/statistics` - Statistical data
- `GET /api/analytics/trends` - Trend analysis
- `GET /api/analytics/prediction` - Predictive analytics

## Diagnostics
- `GET /api/diagnostics/status` - Diagnostics status

## Notes
- All POST endpoints require appropriate request body
- Endpoints that don't exist in ESP32 but may be in frontend:
  - `/api/system/*` endpoints (use direct equivalents above)
  - `/api/backup`, `/api/restore` (not implemented)
  - `/api/factory-reset` (use `/api/reset` for WiFi reset)
  - Various diagnostics endpoints (limited implementation)

## Frontend API Mapping
The frontend ada-api.js has been updated to use these correct endpoints.
- `GET /api/analog-voltage/info` - Sensor information and metadata
- `POST /api/analog-voltage/config` - Update sensor configuration

### Analytics
- `GET /api/analytics/summary` - Analytics summary
- `GET /api/analytics/statistics` - Statistical data
- `GET /api/analytics/trends` - Trend analysis
- `GET /api/analytics/prediction` - Predictive analytics

### Diagnostics
- `GET /api/diagnostics/status` - Diagnostic status
- `GET /api/diagnostics/all` - All diagnostic data
- `GET /api/diagnostics/alerts` - System alerts
- `GET /api/diagnostics/health` - Overall system health

### SD Card
- `GET /api/sd/status` - SD card status
- `GET /api/sd/files` - File listing

### Simulation
- `GET /api/simulation/status` - Simulation status
- `POST /api/simulation/start` - Start simulation
- `POST /api/simulation/stop` - Stop simulation

### Webhooks
- `GET /api/webhooks` - List webhooks
- `POST /api/webhooks` - Create webhook
- `PUT /api/webhooks/{id}` - Update webhook
- `DELETE /api/webhooks/{id}` - Delete webhook
- `GET /api/webhooks/status` - Webhook status
- `GET /api/webhooks/statistics` - Webhook statistics
- `GET /api/webhooks/queue` - Webhook queue

### WebSocket
- `WS /ws` - Real-time data streaming

## Updated API Client

The API client has been updated to use the correct ESP32 endpoints:

### Key Changes:
- `getSensors()` → `/api/analog-voltage`
- `getSystemInfo()` → `/api/status`
- `getSystemDiagnostics()` → `/api/diagnostics/status`
- `getAnalytics()` → `/api/analytics/summary`
- `getAlarms()` → `/api/diagnostics/alerts`

### Proxy Configuration:
```javascript
// astro.config.mjs
proxy: {
  '/api': {
    target: 'http://192.168.111.34',
    changeOrigin: true,
    secure: false
  }
}
```

## Testing:
1. Start Astro dev server: `npm run dev`
2. Open test page: `http://localhost:4321/test.html`
3. Test endpoints to verify proxy is working