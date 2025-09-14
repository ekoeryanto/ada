# Multi-RTU Implementation Guide

## Overview
Implementasi multi-RTU untuk sistem ADA ESP32 yang memungkinkan satu web dashboard mengelola beberapa RTU interface secara bersamaan.

## Architecture

### 1. RTU Client System
- **RTUClient**: Client khusus untuk setiap RTU interface
- **RTUClientManager**: Manager untuk mengelola multiple RTU clients
- **Auto-routing**: Development menggunakan proxy, production menggunakan direct IP

### 2. Proxy Configuration
```javascript
// Development: Multi-RTU proxy endpoints with RTU names
/api          -> Primary RTU (192.168.111.34:80)
/ada-1/api    -> RTU 1 ada-1 (192.168.111.34:80)  
/ada-2/api    -> RTU 2 ada-2 (192.168.111.35:80)
/ada-3/api    -> RTU 3 ada-3 (192.168.111.36:80)
```

### 3. Environment Configuration
```properties
# Primary RTU (ada-1)
RTU_1_NAME=ada-1
RTU_1_HOST=192.168.111.34
RTU_1_PORT=80
RTU_1_INTERFACE_ID=1

# Additional RTUs
RTU_2_NAME=ada-2
RTU_2_HOST=192.168.111.35
RTU_2_PORT=80
RTU_2_INTERFACE_ID=2

RTU_3_NAME=ada-3
RTU_3_HOST=192.168.111.36
RTU_3_PORT=80
RTU_3_INTERFACE_ID=3
```

## Usage

### 1. Using RTU Clients
```typescript
import { RTUClientManager } from './lib/rtu-client';

// Get client for specific RTU by ID (uses ada-X naming)
const ada1Client = RTUClientManager.getRTUClient(1); // ada-1
const ada2Client = RTUClientManager.getRTUClient(2); // ada-2
const ada3Client = RTUClientManager.getRTUClient(3); // ada-3

// Use RTU-specific methods
const status = await ada1Client.getStatus();
const sensors = await ada1Client.getUnifiedSensors();
const devices = await ada1Client.getModbusDevices();
```

### 2. Multi-RTU Operations
```typescript
// Get all RTU statuses
const allStatuses = await RTUClientManager.getAllRTUStatuses();

// Scan all RTUs for devices
const allDevices = await RTUClientManager.scanAllRTUDevices();
```

### 3. RTU Selection in UI
- **Sidebar Selector**: Compact RTU selector in DashboardLayout sidebar
- **Dashboard Integration**: Multi-RTU dashboard with tab-based interface
- **Auto-selection**: Automatically selects ada-1 (RTU 1) by default

## Components

### 1. RTUSidebarSelector.astro
- Compact RTU selector for sidebar
- Real-time status indicators
- Quick actions (scan, view details)
- Auto-selection and persistence

### 2. MultiRTUDashboard.astro
- Multi-RTU overview and management
- Tab-based interface (sensors, modbus, analog, digital, alarms)
- RTU status summary cards
- Individual RTU operations

### 3. RTU Client Library
- **rtu-client.ts**: RTU-specific client implementation
- **esp32-client.ts**: Enhanced primary client with RTU support
- Type consistency across all clients

## Development vs Production

### Development Mode
- Uses Vite dev server proxy
- All RTU requests routed through proxy endpoints
- Automatic endpoint rewriting (`/api-rtu-1` → `/api`)
- Comprehensive logging and error handling

### Production Mode  
- Direct IP connection to each RTU
- Environment variables injected via Astro config
- Fallback mechanisms for offline RTUs
- Production-optimized error handling

## API Endpoints

### Per-RTU Endpoints
Each RTU supports the same API endpoints:

#### Status & System
- `GET /api/status` - System status
- `GET /api/analog-voltage` - Analog voltage readings
- `GET /api/analog-current` - Analog current readings
- `GET /api/digital-io` - Digital I/O status

#### Modbus Operations
- `GET /api/modbus/devices` - List Modbus devices
- `GET /api/modbus/devices/{id}` - Get specific device
- `POST /api/modbus/scan` - Scan for new devices
- `GET /api/modbus/devices/{id}/read/{address}` - Read register
- `POST /api/modbus/devices/{id}/write/{address}` - Write register

#### Unified Sensors
- `GET /api/sensors` - List unified sensors
- `GET /api/sensors/{id}` - Get specific sensor
- `POST /api/sensors/{id}/read` - Read sensor value
- `POST /api/sensors/{id}/enable` - Enable/disable sensor
- `POST /api/sensors/{id}/calibrate` - Calibrate sensor

#### Alarms
- `GET /api/alarms` - List active alarms
- `POST /api/alarms/{id}/acknowledge` - Acknowledge alarm
- `POST /api/alarms/{id}/resolve` - Resolve alarm

## Future Expansion

### Adding New RTUs
1. Add RTU configuration to `.env`:
   ```properties
   RTU_4_NAME=ada-4
   RTU_4_HOST=192.168.111.37
   RTU_4_PORT=80
   RTU_4_INTERFACE_ID=4
   ```

2. Update `astro.config.mjs` to include RTU-4 proxy configuration

3. RTU client system automatically supports any RTU ID

### Enhanced Features
- **RTU Health Monitoring**: Real-time health checks
- **Load Balancing**: Distribute requests across multiple RTUs
- **Failover**: Automatic failover to backup RTUs
- **Performance Analytics**: RTU performance monitoring and analytics

## Troubleshooting

### Common Issues
1. **RTU Not Responding**: Check network connectivity and IP configuration
2. **Proxy Errors**: Verify DEV_PROXY_ENABLED=true and correct RTU hosts
3. **Client Routing**: Ensure correct RTU ID mapping in environment variables

### Debug Tools
- Browser developer console for client-side debugging
- Astro dev server logs for proxy debugging
- Network tab for API request monitoring

## Technical Notes

### Type Safety
- All RTU clients share the same TypeScript interfaces
- Type consistency enforced across main client and RTU clients
- Comprehensive error handling with proper TypeScript types

### Performance
- RTU clients use connection pooling via ofetch
- Automatic retry mechanisms with exponential backoff
- Request/response logging for debugging

### Security
- Environment-based configuration
- No hard-coded credentials or endpoints
- Production/development mode separation