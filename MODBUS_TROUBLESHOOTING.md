# Modbus Altivar 61 Troubleshooting Guide

## Current Issue
- Modbus communication fails with Altivar 61
- All read attempts return `{"success":false}`
- Network statistics show failed requests

## Troubleshooting Steps

### 1. Check Serial Configuration
Test different baud rates and parity settings:

#### A. Baud Rate Testing
- ✅ **9600 baud** (most common) - Currently testing
- ⏳ 19200 baud (tried, failed)
- ⏳ 4800 baud
- ⏳ 38400 baud

#### B. Parity Testing
- ✅ **8-N-1** (No parity) - Currently testing
- ⏳ 8-E-1 (Even parity) - Tried, failed
- ⏳ 8-O-1 (Odd parity)

### 2. Check Slave ID
Test different slave IDs:

```bash
# Test slave ID 1 (Altivar default)
curl -X POST "http://192.168.111.34/api/modbus/read" \
  -H "Content-Type: application/json" \
  -d '{"slave_id": 1, "register_address": 3001, "register_count": 1, "register_type": "input"}'

# Test slave ID 3
curl -X POST "http://192.168.111.34/api/modbus/read" \
  -H "Content-Type: application/json" \
  -d '{"slave_id": 3, "register_address": 3001, "register_count": 1, "register_type": "input"}'
```

### 3. Check Register Address
Test different register types:

```bash
# Try holding registers instead of input registers
curl -X POST "http://192.168.111.34/api/modbus/read" \
  -H "Content-Type: application/json" \
  -d '{"slave_id": 2, "register_address": 3001, "register_count": 1, "register_type": "holding"}'
```

### 4. Physical Connection Check

#### RS485 Wiring
- **A+** (Data+) connected properly
- **B-** (Data-) connected properly
- **Ground** connection if required
- **Termination resistors** (120Ω) at both ends of bus

#### ESP32 Pins
- **RX**: GPIO 16
- **TX**: GPIO 17
- **DE**: GPIO 4 (Direction Enable)

### 5. Altivar 61 Configuration Check

#### Modbus Parameters to Verify
1. **Slave Address**: Usually 1-247
2. **Baud Rate**: 9600, 19200, 38400
3. **Parity**: None, Even, Odd
4. **Stop Bits**: 1 or 2
5. **Modbus Enabled**: Check if Modbus is enabled in VFD

#### Common Altivar 61 Settings
- **Parameter tbd**: Modbus slave address
- **Parameter tbr**: Baud rate
- **Parameter tPr**: Parity

### 6. Test Commands

#### Check if device responds at all
```bash
# Simple connectivity test
curl -X GET "http://192.168.111.34/api/modbus/stats"
```

#### Test with simple register
```bash
# Try a basic register that should always be available
curl -X POST "http://192.168.111.34/api/modbus/read" \
  -H "Content-Type: application/json" \
  -d '{"slave_id": 1, "register_address": 1, "register_count": 1, "register_type": "holding"}'
```

### 7. Current Test Configuration

**Firmware Settings:**
- Baud Rate: 9600
- Format: 8-N-1 (No parity)
- Slave ID: 2
- Pins: RX=16, TX=17, DE=4

**Next Steps:**
1. Upload firmware with 9600 baud
2. Test with slave ID 1, 2, 3
3. Try different register types
4. Check Altivar 61 physical configuration

### 8. Alternative Testing

If all fails, consider:
1. **Modbus simulator** software to test ESP32 Modbus client
2. **Oscilloscope** to check RS485 signals
3. **Different VFD** for testing
4. **ModbusPoll** software to test Altivar 61 directly