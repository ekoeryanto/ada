# ADA-1 API Documentation & Testing

## 📋 Overview

Dokumentasi API lengkap untuk board ADA-1 ESP32-based Modbus dashboard dengan fungsionalitas seperti PLC. Board ini mendukung:

- ✅ **Analog Sensors** (0-10V, 4-20mA)
- ✅ **Modbus RTU Master & Slave**
- ✅ **Real-time Monitoring**
- ✅ **Health Monitoring**
- ✅ **Data Logging to SD Card**
- ✅ **Webhooks & Notifications**
- ✅ **OTA Updates**
- ✅ **Non-blocking PLC-like Operation**

## 📁 Files Included

1. **`API_DOCUMENTATION.md`** - Dokumentasi API lengkap dalam format Markdown
2. **`ADA-1_API_Collection.postman_collection.json`** - Postman collection untuk testing
3. **`ADA-1_Environment.postman_environment.json`** - Environment variables untuk Postman

## 🚀 Quick Start

### 1. Import ke Postman

1. Buka Postman
2. Click **Import** → **Upload Files**
3. Import kedua file JSON:
   - `ADA-1_API_Collection.postman_collection.json`
   - `ADA-1_Environment.postman_environment.json`
4. Pilih environment "ADA-1 Development Environment"
5. Update variable `boardIP` dengan IP address board Anda

### 2. Cari IP Address Board

**Metode 1: mDNS**
```bash
ping ada-1.local
```

**Metode 2: Network Scanner**
```bash
nmap -sn 192.168.1.0/24 | grep ada-1
```

**Metode 3: Router Admin Panel**
- Login ke router
- Cari device dengan hostname "ada-1"

### 3. Test Koneksi

```bash
curl -X GET http://ada-1.local/api/status
```

Atau gunakan request "Get System Status" di Postman collection.

## 🔧 Environment Variables

Update variables berikut di Postman environment:

| Variable | Default Value | Description |
|----------|---------------|-------------|
| `baseUrl` | `http://192.168.1.100/api` | Base API URL |
| `boardIP` | `192.168.1.100` | Board IP address |
| `hostname` | `ada-1.local` | mDNS hostname |
| `modbusSlave` | `1` | Board Modbus slave address |

## 📊 API Endpoints Summary

### System Management
- `GET /api/status` - System status lengkap
- `GET /api/health` - Health monitoring
- `POST /api/restart` - Restart system
- `POST /api/reset` - Reset WiFi settings

### Analog Sensors
- `GET /api/analog-voltage` - All sensor data
- `GET /api/analog-voltage/sensor/{id}` - Specific sensor
- `POST /api/analog-voltage/sensor/{id}/config` - Configure sensor

### Modbus Operations
- `GET /api/modbus/status` - Modbus system status
- `GET /api/modbus/devices` - List devices
- `POST /api/modbus/read` - Read registers
- `POST /api/modbus/write` - Write registers
- `GET /api/modbus/auto-discover` - Auto-discover devices

### Network & WiFi
- `GET /api/wifi/status` - WiFi status
- `GET /api/wifi/scan` - Scan networks
- `POST /api/wifi/connect` - Connect to WiFi

### Data & Analytics
- `GET /api/logs` - Available log files
- `GET /api/analytics` - Analytics data
- `GET /api/webhooks` - Webhook config

## 🧪 Testing Scenarios

### Scenario 1: Basic System Check
1. Get System Status
2. Get Health Status
3. Check all modules are operational

### Scenario 2: Sensor Monitoring
1. Get All Analog Sensors
2. Get individual sensor readings
3. Configure sensor parameters

### Scenario 3: Modbus Communication
1. Get Modbus Status
2. List configured devices
3. Read device registers
4. Auto-discover new devices

### Scenario 4: Network Management
1. Check WiFi status
2. Scan available networks
3. Test connectivity

## 📈 Real-time Data

### WebSocket Connection
```javascript
const ws = new WebSocket('ws://ada-1.local/ws');

ws.onmessage = function(event) {
    const data = JSON.parse(event.data);
    console.log('Real-time data:', data);
};
```

### Example WebSocket Message
```json
{
  "type": "sensor_data",
  "timestamp": 1726308330000,
  "data": {
    "sensor_id": 0,
    "value": 5.25,
    "unit": "bar",
    "status": "OK"
  }
}
```

## 🔍 Troubleshooting

### Connection Issues
```bash
# Test basic connectivity
ping ada-1.local

# Test HTTP response
curl -I http://ada-1.local

# Test API endpoint
curl -X GET http://ada-1.local/api/health
```

### Common Responses

**Success Response:**
```json
{
  "status": "OK",
  "data": { ... }
}
```

**Error Response:**
```json
{
  "error": true,
  "message": "Error description",
  "code": 400
}
```

### Health Status Codes
- `OK` - Module berfungsi normal
- `WARNING` - Module ada masalah minor tapi masih operasional
- `ERROR` - Module ada masalah critical
- `UNKNOWN` - Status module belum diketahui

## 🛠️ Development Tips

### 1. Auto-refresh Environment
Tambahkan pre-request script di Postman:
```javascript
// Auto-update timestamp
pm.globals.set('timestamp', Date.now());

// Auto-detect board IP (jika diperlukan)
if (!pm.environment.get('boardIP')) {
    pm.environment.set('boardIP', '192.168.1.100');
}
```

### 2. Response Validation
Tambahkan test script:
```javascript
pm.test("Status code is 200", function () {
    pm.response.to.have.status(200);
});

pm.test("Response contains data", function () {
    const jsonData = pm.response.json();
    pm.expect(jsonData).to.have.property('data');
});
```

### 3. Batch Testing
Gunakan Postman Runner untuk test seluruh collection secara otomatis.

## 📝 Sample Code

### Python Example
```python
import requests
import json

# Board configuration
BASE_URL = "http://ada-1.local/api"

def get_system_status():
    response = requests.get(f"{BASE_URL}/status")
    return response.json()

def read_modbus_register(device_addr, start_addr, quantity):
    data = {
        "device_address": device_addr,
        "function": 3,
        "start_address": start_addr,
        "quantity": quantity
    }
    response = requests.post(f"{BASE_URL}/modbus/read", json=data)
    return response.json()

# Usage
status = get_system_status()
print(f"System Status: {status['status']}")

modbus_data = read_modbus_register(1, 30001, 2)
print(f"Modbus Data: {modbus_data}")
```

### JavaScript Example
```javascript
const BASE_URL = 'http://ada-1.local/api';

async function getSystemHealth() {
    const response = await fetch(`${BASE_URL}/health`);
    const health = await response.json();
    console.log('System Health:', health);
    return health;
}

async function configureSensor(sensorId, config) {
    const response = await fetch(`${BASE_URL}/analog-voltage/sensor/${sensorId}/config`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    });
    return response.json();
}

// Usage
getSystemHealth();
configureSensor(0, {
    location: "Tank Pressure",
    unit: "bar",
    min_value: 0.0,
    max_value: 10.0,
    enabled: true
});
```

## 🎯 Best Practices

1. **Always check health status** sebelum melakukan operasi critical
2. **Use mDNS hostname** (`ada-1.local`) untuk kemudahan
3. **Monitor response times** - sistem harus respond < 5 detik
4. **Handle errors gracefully** - sistem robust dan non-blocking
5. **Use WebSocket** untuk real-time monitoring
6. **Log all operations** untuk debugging
7. **Test connectivity** sebelum batch operations

## 📞 Support

Untuk bantuan technical atau bug reports:
1. Check health status via `/api/health`
2. Check system logs via `/api/logs`
3. Restart system jika diperlukan via `/api/restart`

---

**Author:** TirtaPatriot  
**Board:** ADA-1 v1.0.0  
**Date:** September 14, 2025