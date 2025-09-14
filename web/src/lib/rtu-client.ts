import { ofetch } from 'ofetch';

// Import types from main ESP32 client to ensure consistency
export type { 
  SensorReading, 
  SystemStatus, 
  ModbusDevice, 
  RTUInterface,
  UnifiedSensorConfig,
  AlarmEvent,
  ModbusNetworkStats,
  ModbusRegister
} from './esp32-client';

// RTU-specific client for individual RTU interfaces
export class RTUClient {
  private client: typeof ofetch;
  private rtuId: number;
  private rtuName: string;
  private baseURL: string;

  constructor(rtuId: number, rtuName?: string) {
    this.rtuId = rtuId;
    this.rtuName = rtuName || `ada-${rtuId}`;
    
    // In development, use RTU-specific proxy endpoints with RTU name
    // In production, use actual RTU addresses
    const isDev = typeof window !== 'undefined' && window.location.hostname === 'localhost';
    
    if (isDev) {
      this.baseURL = `/${this.rtuName}/api`;
    } else {
      // In production, get RTU URL from environment variables injected by Astro
      const rtuUrlKey = `__RTU_${rtuId}_URL__` as keyof Window;
      const rtuUrl = (window as any)[rtuUrlKey];
      this.baseURL = rtuUrl ? `${rtuUrl}/api` : '/api';
    }

    console.log(`[${this.rtuName.toUpperCase()} Client] Mode:`, isDev ? 'Development (using proxy)' : 'Production');
    console.log(`[${this.rtuName.toUpperCase()} Client] Base URL:`, this.baseURL);

    this.client = ofetch.create({
      baseURL: this.baseURL,
      timeout: 10000,
      retry: 3,
      retryDelay: 1000,
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json'
      },
      onRequest: ({ request, options }) => {
        console.log(`[${this.rtuName.toUpperCase()}] → ${options.method || 'GET'} ${request}`);
      },
      onResponse: ({ response }) => {
        console.log(`[${this.rtuName.toUpperCase()}] ← ${response.status}`);
      },
      onResponseError: ({ response }) => {
        console.error(`[${this.rtuName.toUpperCase()}] ✗ ${response.status} ${response.statusText}`);
      }
    });
  }

  // RTU Interface specific methods
  async getStatus() {
    return this.client('/status');
  }

  async getAnalogVoltage() {
    return this.client('/analog-voltage');
  }

  async getAnalogCurrent() {
    return this.client('/analog-current');
  }

  async getDigitalIO() {
    return this.client('/digital-io');
  }

  async getModbusDevices() {
    try {
      return await this.client('/modbus/devices');
    } catch (error) {
      // If modbus/devices endpoint doesn't exist, return empty array
      console.warn(`[${this.rtuName.toUpperCase()}] Modbus devices endpoint not available, returning empty array`);
      return [];
    }
  }

  async getModbusDevice(slaveId: number) {
    try {
      return await this.client(`/modbus/devices/${slaveId}`);
    } catch (error) {
      console.warn(`[${this.rtuName.toUpperCase()}] Modbus device ${slaveId} endpoint not available`);
      return null;
    }
  }

  async readModbusRegister(slaveId: number, address: number) {
    try {
      return await this.client(`/modbus/devices/${slaveId}/read/${address}`);
    } catch (error) {
      console.warn(`[${this.rtuName.toUpperCase()}] Modbus read register endpoint not available`);
      throw error;
    }
  }

  async writeModbusRegister(slaveId: number, address: number, value: number) {
    try {
      return await this.client(`/modbus/devices/${slaveId}/write/${address}`, {
        method: 'POST',
        body: { value }
      });
    } catch (error) {
      console.warn(`[${this.rtuName.toUpperCase()}] Modbus write register endpoint not available`);
      throw error;
    }
  }

  async scanModbusDevices() {
    return this.client('/modbus/scan', {
      method: 'POST'
    });
  }

  // Unified sensor methods
  async getUnifiedSensors() {
    return this.client('/sensors');
  }

  async getUnifiedSensor(sensorId: string) {
    return this.client(`/sensors/${sensorId}`);
  }

  async readUnifiedSensor(sensorId: string) {
    return this.client(`/sensors/${sensorId}/read`, {
      method: 'POST'
    });
  }

  async enableUnifiedSensor(sensorId: string, enabled: boolean) {
    return this.client(`/sensors/${sensorId}/enable`, {
      method: 'POST',
      body: { enabled }
    });
  }

  async calibrateUnifiedSensor(sensorId: string, calibrationData: any) {
    return this.client(`/sensors/${sensorId}/calibrate`, {
      method: 'POST',
      body: calibrationData
    });
  }

  // Alarm methods
  async getAlarms() {
    return this.client('/alarms');
  }

  async acknowledgeAlarm(alarmId: string) {
    return this.client(`/alarms/${alarmId}/acknowledge`, {
      method: 'POST'
    });
  }

  async resolveAlarm(alarmId: string) {
    return this.client(`/alarms/${alarmId}/resolve`, {
      method: 'POST'
    });
  }

  // Utility methods
  getRTUId(): number {
    return this.rtuId;
  }

  getBaseURL(): string {
    return this.baseURL;
  }
}

// RTU Client Manager - manages multiple RTU clients
export class RTUClientManager {
  private static clients: Map<number, RTUClient> = new Map();
  private static rtuNames: Map<number, string> = new Map([
    [1, 'ada-1'],
    [2, 'ada-2'],
    [3, 'ada-3']
  ]);

  static getRTUClient(rtuId: number): RTUClient {
    if (!this.clients.has(rtuId)) {
      const rtuName = this.rtuNames.get(rtuId) || `ada-${rtuId}`;
      this.clients.set(rtuId, new RTUClient(rtuId, rtuName));
    }
    return this.clients.get(rtuId)!;
  }

  static getAllClients(): RTUClient[] {
    return Array.from(this.clients.values());
  }

  static clearClients(): void {
    this.clients.clear();
  }

  // Convenience methods for common RTU operations
  static async getAllRTUStatuses(): Promise<Array<{ rtuId: number; rtuName: string; status: any }>> {
    const results = [];
    for (const [rtuId, client] of this.clients) {
      const rtuName = this.rtuNames.get(rtuId) || `ada-${rtuId}`;
      try {
        const status = await client.getStatus();
        results.push({ rtuId, rtuName, status });
      } catch (error) {
        console.error(`Failed to get status for ${rtuName}:`, error);
        results.push({ rtuId, rtuName, status: { error: error instanceof Error ? error.message : 'Unknown error' } });
      }
    }
    return results;
  }

  static async scanAllRTUDevices(): Promise<Array<{ rtuId: number; rtuName: string; devices: any }>> {
    const results = [];
    for (const [rtuId, client] of this.clients) {
      const rtuName = this.rtuNames.get(rtuId) || `ada-${rtuId}`;
      try {
        const devices = await client.scanModbusDevices();
        results.push({ rtuId, rtuName, devices });
      } catch (error) {
        console.error(`Failed to scan devices for ${rtuName}:`, error);
        results.push({ rtuId, rtuName, devices: { error: error instanceof Error ? error.message : 'Unknown error' } });
      }
    }
    return results;
  }
}

// Create default RTU clients with proper names
export const ada1Client = RTUClientManager.getRTUClient(1); // ada-1
export const ada2Client = RTUClientManager.getRTUClient(2); // ada-2  
export const ada3Client = RTUClientManager.getRTUClient(3); // ada-3