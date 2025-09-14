# ADA-1 Board API Documentation

## Overview
API documentation for ADA-1 ESP32-based Modbus dashboard with PLC-like functionality. The board supports analog sensors, Modbus RTU communication, and real-time monitoring.

**Base URL:** `http://<board-ip>/api`
**Default IP:** Check via WiFi network or use mDNS: `ada-1.local`
**Authentication:** None required for most endpoints

## Table of Contents
1. [System Endpoints](#system-endpoints)
2. [Health Monitoring](#health-monitoring)
3. [Analog Voltage Sensors](#analog-voltage-sensors)
4. [Modbus RTU Operations](#modbus-rtu-operations)
5. [Network & WiFi](#network--wifi)
6. [System Control](#system-control)
7. [Data Logging](#data-logging)
8. [Webhooks](#webhooks)

---

## System Endpoints

### GET /api/status
Get comprehensive system status including all modules and sensors.

**Response Example:**
```json
{
  "project": "ada-1",
  "version": "1.0.0",
  "author": "TirtaPatriot",
  "status": "SYSTEM_RUNNING",
  "uptime": "0d 2h 15m 30s",
  "freeHeap": 187456,
  "chipId": "0x1234567890ABCDEF",
  "wifi": {
    "connected": true,
    "ssid": "YourWiFi",
    "ip": "192.168.1.100",
    "rssi": -45
  },
  "ota": {
    "enabled": true,
    "status": "Ready",
    "url": "http://192.168.1.100:8080/update"
  },
  "sd": {
    "mounted": true,
    "total_mb": 15360,
    "used_mb": 1024,
    "card_type": "SDHC"
  },
  "ntp": {
    "initialized": true,
    "synced": true,
    "last_sync": "2025-09-14 10:30:00",
    "current_time": "2025-09-14 12:45:30",
    "rtc_available": true
  },
  "analog_voltage": {
    "initialized": true,
    "total_readings": 15432,
    "alarm_status": "OK",
    "has_errors": false,
    "sensors": {
      "sensor_0": {
        "location": "Pressure Tank 1",
        "enabled": true,
        "value": 5.25,
        "unit": "bar",
        "voltage": 2.625,
        "raw_voltage": 2.628,
        "status": "OK",
        "health_score": 95,
        "is_dead": false,
        "is_stuck": false,
        "is_calibrated": true,
        "low_alarm": false,
        "high_alarm": false,
        "errors": 0
      }
    }
  }
}
```

---

## Health Monitoring

### GET /api/health
Get health status of all system modules.

**Response Example:**
```json
{
  "wifi": "OK",
  "modbus": "OK",
  "webserver": "OK",
  "sd": "OK",
  "ntp": "OK",
  "ota": "OK",
  "analytics": "OK",
  "diagnostics": "OK",
  "overall_healthy": true,
  "last_check": 1726308330000
}
```

**Health Status Values:**
- `OK`: Module functioning normally
- `WARNING`: Module has minor issues but operational
- `ERROR`: Module has critical issues
- `UNKNOWN`: Module status not determined

---

## Analog Voltage Sensors

### GET /api/analog-voltage
Get detailed analog sensor readings and configuration.

**Response Example:**
```json
{
  "initialized": true,
  "total_readings": 15432,
  "alarm_status": "OK",
  "has_errors": false,
  "sensors": {
    "sensor_0": {
      "location": "Pressure Tank 1",
      "enabled": true,
      "value": 5.25,
      "unit": "bar",
      "voltage": 2.625,
      "raw_voltage": 2.628,
      "quality": 95,
      "timestamp": 1726308330000,
      "identity": {
        "id": "PRESS_TK1_001",
        "manufacturer": "Honeywell",
        "model": "ST3000",
        "serial": "HW123456789"
      },
      "status": "OK",
      "alarms": {
        "low": false,
        "high": false,
        "current_level": 1
      }
    }
  }
}
```

### GET /api/analog-voltage/sensor/{id}
Get specific sensor data (id: 0, 1, or 2).

**Example:** `GET /api/analog-voltage/sensor/0`

### POST /api/analog-voltage/sensor/{id}/config
Configure sensor parameters.

**Request Body:**
```json
{
  "location": "Pressure Tank 1",
  "unit": "bar",
  "min_value": 0.0,
  "max_value": 10.0,
  "low_alarm": 2.0,
  "high_alarm": 8.0,
  "enabled": true
}
```

---

## Modbus RTU Operations

### GET /api/modbus/status
Get Modbus system status and device count.

**Response Example:**
```json
{
  "initialized": true,
  "master_enabled": true,
  "slave_enabled": true,
  "slave_address": 1,
  "baud_rate": 9600,
  "total_devices": 2,
  "total_registers": 8,
  "communication_errors": 0,
  "last_scan": 1726308330000
}
```

### GET /api/modbus/devices
List all configured Modbus devices.

**Response Example:**
```json
{
  "devices": [
    {
      "id": 1,
      "name": "Energy Meter Main",
      "address": 1,
      "enabled": true,
      "last_communication": 1726308330000,
      "status": "ONLINE",
      "error_count": 0,
      "response_time_ms": 45
    }
  ]
}
```

### GET /api/modbus/devices/{id}
Get specific device details and register values.

**Example:** `GET /api/modbus/devices/1`

**Response Example:**
```json
{
  "device": {
    "id": 1,
    "name": "Energy Meter Main",
    "address": 1,
    "enabled": true,
    "status": "ONLINE"
  },
  "registers": [
    {
      "address": 30001,
      "name": "Voltage L1",
      "value": 235.5,
      "unit": "V",
      "quality": 100,
      "timestamp": 1726308330000
    }
  ]
}
```

### POST /api/modbus/devices
Add new Modbus device.

**Request Body:**
```json
{
  "name": "New Energy Meter",
  "address": 2,
  "enabled": true,
  "registers": [
    {
      "address": 30001,
      "name": "Voltage",
      "unit": "V",
      "data_type": "float",
      "read_interval": 5000
    }
  ]
}
```

### PUT /api/modbus/devices/{id}
Update device configuration.

### DELETE /api/modbus/devices/{id}
Remove device from configuration.

### POST /api/modbus/read
Read specific Modbus registers.

**Request Body:**
```json
{
  "device_address": 1,
  "function": 3,
  "start_address": 30001,
  "quantity": 2
}
```

### POST /api/modbus/write
Write to Modbus registers.

**Request Body:**
```json
{
  "device_address": 1,
  "function": 6,
  "address": 40001,
  "value": 1234
}
```

### GET /api/modbus/auto-discover
Auto-discover Modbus devices on the network.

**Query Parameters:**
- `start_address`: Starting slave address (default: 1)
- `end_address`: Ending slave address (default: 247)
- `timeout`: Timeout per device in ms (default: 1000)

---

## Network & WiFi

### GET /api/wifi/status
Get WiFi connection status.

**Response Example:**
```json
{
  "connected": true,
  "ssid": "YourWiFi",
  "ip": "192.168.1.100",
  "gateway": "192.168.1.1",
  "subnet": "255.255.255.0",
  "dns1": "8.8.8.8",
  "rssi": -45,
  "channel": 6,
  "encryption": "WPA2"
}
```

### GET /api/wifi/scan
Scan for available WiFi networks.

**Response Example:**
```json
{
  "networks": [
    {
      "ssid": "WiFi_Network_1",
      "rssi": -45,
      "encryption": "WPA2",
      "channel": 6
    }
  ]
}
```

### POST /api/wifi/connect
Connect to WiFi network.

**Request Body:**
```json
{
  "ssid": "YourWiFi",
  "password": "your_password"
}
```

---

## System Control

### POST /api/restart
Restart the system.

**Response:**
```json
{
  "message": "Restarting..."
}
```

### POST /api/reset
Reset WiFi settings and restart.

**Response:**
```json
{
  "message": "Resetting WiFi settings..."
}
```

### GET /api/time
Get current system time.

**Response Example:**
```json
{
  "timestamp": 1726308330000,
  "iso_string": "2025-09-14T12:45:30.000Z",
  "local_time": "2025-09-14 19:45:30",
  "timezone": "WIB",
  "ntp_synced": true,
  "rtc_available": true
}
```

### POST /api/time/sync
Force NTP time synchronization.

---

## Data Logging

### GET /api/logs
Get available log files.

**Response Example:**
```json
{
  "logs": [
    {
      "name": "sensors_2025-09-14.csv",
      "size": 1024000,
      "created": "2025-09-14T00:00:00.000Z"
    }
  ]
}
```

### GET /api/logs/{filename}
Download specific log file.

**Example:** `GET /api/logs/sensors_2025-09-14.csv`

### GET /api/analytics
Get analytics and trends data.

**Response Example:**
```json
{
  "sensor_trends": {
    "sensor_0": {
      "average_1h": 5.2,
      "min_1h": 4.8,
      "max_1h": 5.6,
      "trend": "stable",
      "prediction_1h": 5.3
    }
  },
  "system_performance": {
    "avg_response_time": 45,
    "error_rate": 0.02,
    "uptime_percentage": 99.8
  }
}
```

---

## Webhooks

### GET /api/webhooks
Get webhook configuration.

**Response Example:**
```json
{
  "enabled": true,
  "endpoints": [
    {
      "name": "Main Server",
      "url": "https://your-server.com/webhook",
      "enabled": true,
      "events": ["sensor_alarm", "device_offline"]
    }
  ]
}
```

### POST /api/webhooks
Configure webhook endpoint.

**Request Body:**
```json
{
  "name": "Main Server",
  "url": "https://your-server.com/webhook",
  "enabled": true,
  "events": ["sensor_alarm", "device_offline", "system_error"],
  "headers": {
    "Authorization": "Bearer your-token"
  }
}
```

### POST /api/webhooks/test
Test webhook endpoint.

**Request Body:**
```json
{
  "url": "https://your-server.com/webhook"
}
```

---

## Error Responses

All endpoints may return error responses in this format:

```json
{
  "error": true,
  "message": "Error description",
  "code": 400
}
```

**Common HTTP Status Codes:**
- `200`: Success
- `400`: Bad Request
- `404`: Not Found
- `500`: Internal Server Error

---

## Postman Collection

You can import this API into Postman using the following collection:

```json
{
  "info": {
    "name": "ADA-1 Board API",
    "description": "API collection for ADA-1 ESP32 Modbus Dashboard",
    "version": "1.0.0"
  },
  "variable": [
    {
      "key": "baseUrl",
      "value": "http://192.168.1.100/api",
      "description": "Base URL for ADA-1 board"
    }
  ],
  "item": [
    {
      "name": "System Status",
      "request": {
        "method": "GET",
        "url": "{{baseUrl}}/status"
      }
    },
    {
      "name": "Health Check",
      "request": {
        "method": "GET",
        "url": "{{baseUrl}}/health"
      }
    },
    {
      "name": "Analog Sensors",
      "request": {
        "method": "GET",
        "url": "{{baseUrl}}/analog-voltage"
      }
    },
    {
      "name": "Modbus Devices",
      "request": {
        "method": "GET",
        "url": "{{baseUrl}}/modbus/devices"
      }
    }
  ]
}
```

---

## Usage Examples

### curl Examples

**Get system status:**
```bash
curl -X GET http://ada-1.local/api/status
```

**Read Modbus register:**
```bash
curl -X POST http://ada-1.local/api/modbus/read \
  -H "Content-Type: application/json" \
  -d '{
    "device_address": 1,
    "function": 3,
    "start_address": 30001,
    "quantity": 2
  }'
```

**Configure sensor:**
```bash
curl -X POST http://ada-1.local/api/analog-voltage/sensor/0/config \
  -H "Content-Type: application/json" \
  -d '{
    "location": "Tank Pressure",
    "unit": "bar",
    "min_value": 0.0,
    "max_value": 10.0,
    "enabled": true
  }'
```

---

## Real-time Updates

The board also supports WebSocket connections for real-time data:

**WebSocket Endpoint:** `ws://<board-ip>/ws`

**Message Format:**
```json
{
  "type": "sensor_data",
  "timestamp": 1726308330000,
  "data": {
    "sensor_id": 0,
    "value": 5.25,
    "unit": "bar"
  }
}
```

---

## Notes

1. **Network Discovery:** Use mDNS hostname `ada-1.local` if DHCP IP is unknown
2. **Data Format:** All timestamps are Unix milliseconds
3. **Sensor IDs:** Analog sensors are numbered 0, 1, 2
4. **Modbus Addressing:** Uses standard Modbus register addressing
5. **Real-time:** WebSocket provides live sensor updates every 2 seconds
6. **Logging:** Data is logged to SD card in CSV format
7. **Health Monitoring:** System performs health checks every 5 seconds

---

*Generated for ADA-1 Board v1.0.0 - TirtaPatriot*