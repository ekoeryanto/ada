# Altivar 61 VFD Monitoring Configuration

## Overview
ESP32 system configured for **monitoring only** of Schneider Electric Altivar 61 Variable Frequency Drive (VFD).

> **⚠️ IMPORTANT: This system is for MONITORING purposes only. No control commands are sent to the VFD.**

## Communication Configuration

### Modbus RTU Settings
- **Baud Rate**: 19,200 bps
- **Data Format**: 8-E-1 (8 data bits, Even parity, 1 stop bit)
- **Slave ID**: 2
- **Protocol**: Modbus RTU over RS485

### ESP32 Hardware Pins
- **RS485 RX**: GPIO 16
- **RS485 TX**: GPIO 17  
- **RS485 DE**: GPIO 4 (Direction Enable)

## Monitored Parameters

### 1. Status Word (RUN, STOP, FAULT)
- **Register**: 3001 (0x0BB9)
- **Data Type**: UINT16
- **Unit**: Status code
- **Description**: Drive operational status (RUN, STOP, FAULT, etc.)
- **Update Interval**: 2 seconds

### 2. Output Speed Feedback
- **Register**: 3202 (0x0C82)
- **Data Type**: UINT16
- **Unit**: RPM (0.1 RPM resolution)
- **Range**: 0.0 - 6000.0 RPM
- **Update Interval**: 3 seconds

### 3. Motor Current
- **Register**: 3204 (0x0C84)
- **Data Type**: UINT16
- **Unit**: A (0.1 A resolution)
- **Range**: 0.0 - 500.0 A
- **Update Interval**: 3 seconds

### 4. Drive Thermal State
- **Register**: 3303 (0x0CE7)
- **Data Type**: UINT16
- **Unit**: % (thermal capacity)
- **Range**: 0.0 - 120.0 %
- **Update Interval**: 10 seconds

### 5. Fault Code
- **Register**: 3027 (0x0BD3)
- **Data Type**: UINT16
- **Unit**: Error code
- **Range**: 0 - 65535
- **Update Interval**: 5 seconds

## API Endpoints

### Get Device Status
```bash
GET /api/modbus/devices
```

### Read Specific Register
```bash
# Read Status Word
POST /api/modbus/read
Content-Type: application/json

{
  "slave_id": 2,
  "register_address": 3001,
  "register_count": 1,
  "register_type": "input"
}

# Read Output Speed
POST /api/modbus/read
Content-Type: application/json

{
  "slave_id": 2,
  "register_address": 3202,
  "register_count": 1,
  "register_type": "input"
}

# Read Motor Current
POST /api/modbus/read
Content-Type: application/json

{
  "slave_id": 2,
  "register_address": 3204,
  "register_count": 1,
  "register_type": "input"
}

# Read Fault Code
POST /api/modbus/read
Content-Type: application/json

{
  "slave_id": 2,
  "register_address": 3027,
  "register_count": 1,
  "register_type": "input"
}
```

### Get Statistics
```bash
GET /api/modbus/stats
```

## Web Interface
Access monitoring dashboard at: `http://[ESP32_IP]/modbus`

## Safety Notes
1. **Read-Only**: All register mappings are configured as `readOnly: true`
2. **No Control**: System cannot send control commands to VFD
3. **Monitoring Only**: Pure data acquisition system
4. **Fail-Safe**: If communication fails, VFD continues normal operation

## Register Value Interpretation

### Status Word (Register 3001) Bit Mapping
| Bit | Description | Value |
|-----|-------------|-------|
| 0 | Ready to switch on | 0=Not ready, 1=Ready |
| 1 | Switched on | 0=Not switched on, 1=Switched on |
| 2 | Operation enabled | 0=Disabled, 1=Enabled |
| 3 | Fault | 0=No fault, 1=Fault |
| 4 | Voltage enabled | 0=Disabled, 1=Enabled |
| 5 | Quick stop | 0=Quick stop, 1=No quick stop |
| 6 | Switch on disabled | 0=Enabled, 1=Disabled |
| 7 | Warning | 0=No warning, 1=Warning |
| 8 | Manufacturer specific | - |
| 9 | Remote | 0=Local, 1=Remote |
| 10 | Target reached | 0=Not reached, 1=Reached |
| 11 | Internal limit active | 0=No limit, 1=Limit active |

### Common Status Word Values
- **0x0001**: Ready to switch on
- **0x0003**: Switched on
- **0x0007**: Operation enabled (Running)
- **0x0008**: Fault condition
- **0x0010**: Quick stop active

### Fault Codes (Register 3027) - Common Values
- **0**: No fault
- **1**: Overcurrent
- **2**: Overvoltage
- **3**: Undervoltage
- **4**: Overtemperature motor
- **5**: Overtemperature drive
- **6**: Motor overload
- **7**: Ground fault
- **And more...** (Refer to Altivar 61 manual for complete list)

## Troubleshooting

### Device Shows "Offline"
1. Check physical RS485 connections
2. Verify Altivar 61 Modbus settings match:
   - Baud rate: 19200
   - Format: 8-E-1
   - Slave ID: 2
3. Check RS485 termination resistors
4. Verify ESP32 pin connections

### Communication Errors
1. Check RS485 wiring polarity (A+, B-)
2. Ensure proper grounding
3. Verify register addresses are correct for your Altivar 61 model
4. Check for electromagnetic interference

## Altivar 61 Configuration
Ensure your Altivar 61 is configured with:
- Modbus enabled
- Slave address: 2
- Baud rate: 19200
- Parity: Even
- Stop bits: 1

## Data Visualization
Monitored data can be viewed through:
1. Web dashboard (`/modbus`)
2. Real-time API endpoints
3. Analytics page (`/analytics`) for historical trends
4. Webhook notifications for alerts