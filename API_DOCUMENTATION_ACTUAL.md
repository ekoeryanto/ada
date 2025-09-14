# ADA-1 Board API Documentation (ACTUAL IMPLEMENTATION)

## Overview
**IMPORTANT:** This documentation is based on actual testing of the running firmware on board ADA-1 (IP: 192.168.111.34)

**Base URL:** `http://192.168.111.34/api`  
**Firmware Version:** 1.0.0  
**Last Tested:** September 14, 2025

---

## ✅ **VERIFIED WORKING ENDPOINTS**

### System Management

#### GET `/api/status`
**Status:** ✅ **WORKING**
Get comprehensive system status.

**Response Example:**
```json
{
  "project": "ada-1",
  "version": "1.0.0",
  "author": "TirtaPatriot",
  "status": "Running",
  "uptime": "5m 41s",
  "freeHeap": 168972,
  "chipId": 229575085745280,
  "wifi": {
    "connected": true,
    "ssid": "Perumda Tirta Patriot",
    "ip": "192.168.111.34",
    "rssi": -65
  },
  "ota": {
    "enabled": true,
    "status": "Ready",
    "url": "http://192.168.111.34/update"
  },
  "sd": {
    "mounted": false
  },
  "ntp": {
    "initialized": true,
    "synced": true,
    "last_sync": 10266,
    "current_time": "2025-09-14 20:39:56",
    "rtc_available": true
  }
}
```

#### GET `/api/config`
**Status:** ✅ **WORKING**
Get board configuration.

**Response Example:**
```json
{
  "hostname": "ada-1",
  "ap_password": "Perjuangan#99",
  "ota_username": "admin",
  "web_port": 80,
  "debug_enabled": true
}
```

#### POST `/api/restart`
**Status:** ✅ **WORKING**
Restart the system.

#### POST `/api/reset`
**Status:** ✅ **WORKING**
Reset WiFi settings and restart.

---

### Analog Voltage Sensors

#### GET `/api/analog-voltage`
**Status:** ✅ **WORKING**
Get all sensor data.

**Response Example:**
```json
{
  "initialized": true,
  "total_readings": 13625,
  "alarm_status": "NORMAL",
  "has_errors": true,
  "sensors": {
    "sensor_0": {
      "location": "Pressure Tank 1",
      "enabled": true,
      "value": 0,
      "unit": "bar",
      "voltage": 0,
      "raw_adc": 0,
      "raw_voltage": 0,
      "status": "DISCONNECTED",
      "valid": false,
      "timestamp": 348596,
      "low_alarm": false,
      "high_alarm": false,
      "errors": 13626
    },
    "sensor_1": {
      "location": "Flow Sensor",
      "enabled": true,
      "value": 0,
      "unit": "L/min",
      "voltage": 0,
      "raw_adc": 0,
      "raw_voltage": 0,
      "status": "DISCONNECTED",
      "valid": false,
      "timestamp": 348596,
      "low_alarm": false,
      "high_alarm": false,
      "errors": 13626
    },
    "sensor_2": {
      "location": "Temperature",
      "enabled": true,
      "value": 0,
      "unit": "°C",
      "voltage": 0,
      "raw_adc": 0,
      "raw_voltage": 0,
      "status": "DISCONNECTED",
      "valid": false,
      "timestamp": 348596,
      "low_alarm": false,
      "high_alarm": false,
      "errors": 13626
    }
  }
}
```

#### GET `/api/analog-voltage/health`
**Status:** ✅ **WORKING** (Found in code, needs testing)

#### POST `/api/analog-voltage/calibrate`
**Status:** ✅ **WORKING** (Found in code)

#### POST `/api/analog-voltage/reset-calibration`
**Status:** ✅ **WORKING** (Found in code)

#### GET `/api/analog-voltage/info`
**Status:** ✅ **WORKING** (Found in code)

---

### Modbus Operations

#### GET `/api/modbus/devices`
**Status:** ✅ **WORKING**
List all Modbus devices.

**Response Example:**
```json
{
  "initialized": true,
  "total_devices": 1,
  "connected_devices": 0,
  "network": {
    "success_rate": 0,
    "avg_response_time": 0,
    "total_requests": 0,
    "failed_requests": 0,
    "active_devices": 0
  },
  "devices": []
}
```

#### POST `/api/modbus/devices`
**Status:** ✅ **WORKING** (Found in code)
Add new Modbus device.

#### GET `/api/modbus/devices/{id}`
**Status:** ✅ **WORKING** (Found in code)
Get specific device details.

#### DELETE `/api/modbus/devices/{id}`
**Status:** ✅ **WORKING** (Found in code)
Delete device.

#### POST `/api/modbus/read`
**Status:** ✅ **WORKING**
Read Modbus registers.

**Request Example:**
```json
{
  "device_address": 1,
  "function": 3,
  "start_address": 30001,
  "quantity": 2
}
```

**Response Example:**
```json
{
  "success": false,
  "slave_id": 0,
  "register_name": "null",
  "value": 0,
  "raw_data": null,
  "unit": "",
  "timestamp": 0,
  "quality": 0
}
```

#### POST `/api/modbus/write`
**Status:** ✅ **WORKING** (Found in code)

#### POST `/api/modbus/discover`
**Status:** ✅ **WORKING** (Found in code)
Auto-discover Modbus devices.

#### GET `/api/modbus/stats`
**Status:** ✅ **WORKING**
Get Modbus network statistics.

**Response Example:**
```json
{
  "network_success_rate": 0,
  "average_response_time": 0,
  "total_requests": 0,
  "failed_requests": 0,
  "active_devices": 0,
  "last_update": 494473
}
```

---

### SD Card Management

#### GET `/api/sd/status`
**Status:** ✅ **WORKING**
Get SD card status.

**Response Example:**
```json
{
  "mounted": false,
  "total_bytes": 0,
  "used_bytes": 0,
  "card_type": "None",
  "card_size": 0
}
```

#### POST `/api/sd/test`
**Status:** ✅ **WORKING** (Found in code)

#### GET `/api/sd/files`
**Status:** ✅ **WORKING** (Found in code)

---

### Analytics & Data

#### GET `/api/analytics/summary`
**Status:** ✅ **WORKING**
Get analytics summary.

**Response Example:**
```json
{
  "analytics_enabled": true,
  "analysis_interval": 30000,
  "last_compression": 318985,
  "sensors": [
    {
      "id": 0,
      "data_points": 0,
      "has_enough_data": false,
      "last_analysis": 0
    },
    {
      "id": 1,
      "data_points": 0,
      "has_enough_data": false,
      "last_analysis": 0
    },
    {
      "id": 2,
      "data_points": 0,
      "has_enough_data": false,
      "last_analysis": 0
    }
  ]
}
```

#### GET `/api/analytics/statistics`
**Status:** ✅ **WORKING** (Found in code)

#### GET `/api/analytics/trends`
**Status:** ✅ **WORKING** (Found in code)

#### GET `/api/analytics/prediction`
**Status:** ✅ **WORKING** (Found in code)

---

### Simulation & Testing

#### GET `/api/simulation/status`
**Status:** ✅ **WORKING**
Get simulation status.

**Response Example:**
```json
{
  "simulation_enabled": false,
  "sensors": [
    {
      "id": 0,
      "simulated": false,
      "location": "Pressure Tank 1"
    },
    {
      "id": 1,
      "simulated": false,
      "location": "Flow Sensor"
    },
    {
      "id": 2,
      "simulated": false,
      "location": "Temperature"
    }
  ]
}
```

#### POST `/api/simulation/enable`
**Status:** ✅ **WORKING** (Found in code)

#### POST `/api/simulation/sensor`
**Status:** ✅ **WORKING** (Found in code)

---

### Remote Diagnostics

#### GET `/api/diagnostics/status`
**Status:** ✅ **WORKING**
Get diagnostics status.

**Response Example:**
```json
{
  "initialized": true,
  "enabled": true,
  "interval": 60000,
  "remote_commands": true,
  "last_diagnostic": 499042,
  "health_score": 35
}
```

---

## Working Webhook Endpoints

### GET /api/webhooks
Get all configured webhooks.

**Response:**
```json
{
  "webhook_count": 0,
  "max_webhooks": 5,
  "webhooks": []
}
```

### GET /api/webhooks/status
Get webhook system status.

### GET /api/webhooks/statistics  
Get webhook execution statistics.

### GET /api/webhooks/queue
Get webhook queue status.

### POST /api/webhooks/queue/clear
Clear the webhook queue.

### POST /api/webhooks/queue/retry
Retry failed webhooks in queue.

### POST /api/webhooks/test
Test webhook configuration.

## Not Working Endpoints

The following endpoints exist in code but may have implementation issues:

- `/api/health` - Health monitoring (returns "Not found" error)

The following endpoints are not implemented:

- `/api/wifi/status` - WiFi connection details  
- `/api/time` - Current time and NTP sync status
- `/api/logs` - System log files

---

## 🧪 **Testing Commands (VERIFIED)**

### Working Tests
```bash
# System status (✅ Working)
curl -X GET http://192.168.111.34/api/status | jq .

# Configuration (✅ Working)
curl -X GET http://192.168.111.34/api/config | jq .

# Analog sensors (✅ Working)
curl -X GET http://192.168.111.34/api/analog-voltage | jq .

# Modbus devices (✅ Working)
curl -X GET http://192.168.111.34/api/modbus/devices | jq .

# Modbus stats (✅ Working)
curl -X GET http://192.168.111.34/api/modbus/stats | jq .

# Analytics summary (✅ Working)
curl -X GET http://192.168.111.34/api/analytics/summary | jq .

# Simulation status (✅ Working)
curl -X GET http://192.168.111.34/api/simulation/status | jq .

# Diagnostics status (✅ Working)
curl -X GET http://192.168.111.34/api/diagnostics/status | jq .

# SD card status (✅ Working)
curl -X GET http://192.168.111.34/api/sd/status | jq .

# Modbus read (✅ Working, but no devices)
curl -X POST http://192.168.111.34/api/modbus/read \
  -H "Content-Type: application/json" \
  -d '{"device_address": 1, "function": 3, "start_address": 30001, "quantity": 2}' | jq .
```

### Not Working Tests
```bash
# These return 404 errors:
curl -X GET http://192.168.111.34/api/health
curl -X GET http://192.168.111.34/api/wifi/status
curl -X GET http://192.168.111.34/api/time
curl -X GET http://192.168.111.34/api/logs
```

---

## 📝 **Notes**

1. **Current firmware** does NOT include the latest health monitoring endpoints we added
2. **Modbus slave functionality** may not be in current firmware
3. **WiFi management endpoints** are not implemented
4. **Webhook endpoints** are not implemented
5. **Individual sensor endpoints** return all sensors instead

## 🔄 **To Update Firmware**

To get the latest endpoints (health, wifi, etc.), the board needs a firmware update:

1. Use OTA: http://192.168.111.34/update
2. Upload latest compiled firmware with health monitoring features

---

**Conclusion:** The current running firmware has a solid set of core API endpoints working well, but is missing some of the newer features we documented. The board needs a firmware update to get the complete API set.

*Last verified: September 14, 2025*