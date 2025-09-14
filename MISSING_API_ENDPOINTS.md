# 🚨 CRITICAL: MISSING API ENDPOINTS DISCOVERED

## Issue: Analog Current (4-20mA) Manager NOT EXPOSED via API

### Current Status:
- ✅ `AnalogCurrentManager` exists and implemented
- ✅ Initialized in `main.cpp` with 3 sensors:
  - Sensor 0: "Level Tank 2" (mm) - Rosemount 3051L
  - Sensor 1: "Flow Line 2" (L/min) - Yokogawa ADMAG AE  
  - Sensor 2: "Pressure Sys" (bar) - Endress+Hauser Cerabar PMC21
- ✅ Running in main loop (`analogCurrentMgr.handle()`)
- ❌ **NO API ENDPOINTS** in `web_server.cpp`

### Missing API Endpoints:
1. `GET /api/analog-current` - Get all 4-20mA sensor readings
2. `GET /api/analog-current/health` - Current loop health status
3. `GET /api/analog-current/info` - Sensor information & diagnostics
4. `GET /api/analog-current/diagnostics` - Loop diagnostics & resistance
5. `POST /api/analog-current/calibrate` - Calibrate current sensors
6. `POST /api/analog-current/reset-calibration` - Reset calibration

### Required Fix:
Add `#include "analog_current_manager.h"` to `web_server.cpp` and implement endpoints similar to `analog-voltage` endpoints.

### Impact:
- **Major functionality missing** - 4-20mA sensors cannot be monitored via API
- Professional industrial sensors (Rosemount, Yokogawa, E+H) not accessible
- Current loop diagnostics unavailable
- Loop resistance monitoring not exposed

### Priority: 🔴 HIGH
This is a significant oversight in industrial monitoring capability.