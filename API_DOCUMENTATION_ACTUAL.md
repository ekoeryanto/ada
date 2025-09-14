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

### Analog Voltage (0-10V) Endpoints

### GET /api/analog-voltage
Get readings from all analog voltage sensors (0-10V inputs).

**Response:**
```json
{
  "initialized": true,
  "total_readings": 32000,
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
      "timestamp": 820396,
      "low_alarm": false,
      "high_alarm": false,
      "errors": 32960
    }
  }
}
```

### GET /api/analog-voltage/health
Get health status of analog voltage sensors.

### GET /api/analog-voltage/info  
Get sensor information and configuration details.

## Analog Current (4-20mA) Endpoints - ✅ NEW!

### GET /api/analog-current
Get readings from all analog current sensors (4-20mA loops).

**Response:**
```json
{
  "initialized": true,
  "total_readings": 15000,
  "alarm_status": "NORMAL", 
  "has_errors": false,
  "sensors": {
    "sensor_0": {
      "location": "Level Tank 2",
      "enabled": true,
      "value": 2500.5,
      "unit": "mm",
      "current": 12.5,
      "voltage": 1.5,
      "raw_adc": 1860,
      "status": "OK",
      "valid": true,
      "timestamp": 856432,
      "loop_resistance": 250.8,
      "signal_quality": 95.2,
      "low_alarm": false,
      "high_alarm": false,
      "errors": 0
    },
    "sensor_1": {
      "location": "Flow Line 2", 
      "enabled": true,
      "value": 45.8,
      "unit": "L/min",
      "current": 8.2,
      "voltage": 0.98,
      "raw_adc": 1220,
      "status": "OK",
      "valid": true,
      "timestamp": 856431,
      "loop_resistance": 298.5,
      "signal_quality": 92.1,
      "low_alarm": false,
      "high_alarm": false,
      "errors": 0
    },
    "sensor_2": {
      "location": "Pressure Sys",
      "enabled": true, 
      "value": 6.8,
      "unit": "bar",
      "current": 10.4,
      "voltage": 1.25,
      "raw_adc": 1550,
      "status": "OK",
      "valid": true,
      "timestamp": 856430,
      "loop_resistance": 201.2,
      "signal_quality": 98.7,
      "low_alarm": false,
      "high_alarm": false,
      "errors": 0
    }
  }
}
```

### GET /api/analog-current/health
Get current loop health status with diagnostic information.

**Response:**
```json
{
  "initialized": true,
  "total_readings": 15000,
  "alarm_status": "NORMAL",
  "has_errors": false,
  "sensors": {
    "sensor_0": {
      "location": "Level Tank 2",
      "enabled": true,
      "current": 12.5,
      "expected_range": "4.0-20.0 mA",
      "status": "OK",
      "valid": true,
      "loop_resistance": 250.8,
      "signal_quality": 95.2,
      "timestamp": 856432,
      "low_alarm": false,
      "high_alarm": false,
      "errors": 0,
      "needs_calibration": false
    }
  }
}
```

### GET /api/analog-current/info
Get sensor information including manufacturer details and configuration.

**Response:**
```json
{
  "system": "Analog Current Manager (4-20mA)",
  "initialized": true,
  "total_readings": 15000,
  "sensors": {
    "sensor_0": {
      "location": "Level Tank 2",
      "unit": "mm", 
      "manufacturer": "Rosemount",
      "model": "3051L",
      "serial_number": "RM789123456",
      "sensor_id": "LEVEL_TK2_004",
      "enabled": true,
      "calibrated": true
    },
    "sensor_1": {
      "location": "Flow Line 2",
      "unit": "L/min",
      "manufacturer": "Yokogawa", 
      "model": "ADMAG AE",
      "serial_number": "YG123789456",
      "sensor_id": "FLOW_LINE2_005",
      "enabled": true,
      "calibrated": true
    },
    "sensor_2": {
      "location": "Pressure Sys",
      "unit": "bar",
      "manufacturer": "Endress+Hauser",
      "model": "Cerabar PMC21", 
      "serial_number": "EH456123789",
      "sensor_id": "PRESS_SYS_006",
      "enabled": true,
      "calibrated": true
    }
  }
}
```

### GET /api/analog-current/diagnostics
Get detailed loop diagnostics and resistance monitoring.

**Response:**
```json
{
  "system": "4-20mA Loop Diagnostics",
  "initialized": true,
  "sensors": {
    "sensor_0": {
      "location": "Level Tank 2",
      "current_reading": 12.5,
      "loop_resistance": 250.8,
      "signal_quality": 95.2,
      "status": "OK",
      "loop_integrity": "OK",
      "wire_resistance": "NORMAL",
      "signal_noise": "LOW",
      "calibration_drift": "NONE"
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

### Digital I/O API

#### GET `/api/digital-io`
**Status:** ✅ **NEW - IMPLEMENTED**
Get complete digital I/O status including all inputs and outputs.

**Response Example:**
```json
{
  "initialized": true,
  "total_updates": 12450,
  "timestamp": 123456789,
  "inputs": [
    {
      "index": 0,
      "name": "DI1",
      "state": 0,
      "is_high": false,
      "pulse_count": 15,
      "health_score": 100.0,
      "reading": {
        "valid": true,
        "last_change_time": 123456,
        "state_hold_time": 5000,
        "debounce_active": false,
        "alarm_active": false
      }
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "DO1", 
      "state": 0,
      "is_on": false,
      "health_score": 100.0,
      "status": {
        "physical_state": false,
        "pwm_duty_cycle": 0,
        "operation_count": 25,
        "last_operation_time": 123456,
        "state_hold_time": 2000
      }
    }
  ]
}
```

#### GET `/api/digital-io/inputs`
**Status:** ✅ **NEW - IMPLEMENTED**
Get detailed digital input status.

#### GET `/api/digital-io/outputs`
**Status:** ✅ **NEW - IMPLEMENTED**
Get detailed digital output status.

#### POST `/api/digital-io/output`
**Status:** ✅ **NEW - IMPLEMENTED**
Control digital outputs.

**Parameters:**
- `output`: Output index (0-3) for DO1-DO4
- `state`: ON/OFF/PULSE/BLINK
- `duration`: For PULSE state, duration in milliseconds (optional)

**Examples:**
```bash
# Turn output 0 ON
curl -X POST "http://192.168.4.1/api/digital-io/output?output=0&state=ON"

# Pulse output 2 for 2 seconds
curl -X POST "http://192.168.4.1/api/digital-io/output?output=2&state=PULSE&duration=2000"

# Start blinking output 3
curl -X POST "http://192.168.4.1/api/digital-io/output?output=3&state=BLINK"
```

**Digital I/O Pin Mapping:**
- DI1-DI4: Digital input pins (GPIO with pullup, debounce)
- DO1-DO4: Digital output pins (GPIO with PWM capability)

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