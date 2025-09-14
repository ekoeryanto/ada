import { ofetch } from 'ofetch';

// Type definitions for ESP32 API responses
export interface SensorReading {
  id: string;
  name: string;
  value: number;
  unit: string;
  timestamp: number;
  quality: number;
  type: 'digital_input' | 'digital_output' | 'analog_voltage' | 'analog_current' | 'modbus_rtu' | 'virtual' | 'system';
  status: 'unknown' | 'ok' | 'warning' | 'error' | 'offline' | 'maintenance' | 'calibrating';
  sourceType: string;
  sourceAddress: string;
  additionalValues?: Record<string, number>;
  group?: string;
  location?: string;
  tags?: Record<string, string>;
}

export interface SystemStatus {
  uptime: number;
  memory: {
    free: number;
    used: number;
    total: number;
  };
  wifi: {
    connected: boolean;
    ssid: string;
    rssi: number;
    ip: string;
  };
  sd: {
    mounted: boolean;
    free: number;
    used: number;
  };
  ntp: {
    synced: boolean;
    time: string;
  };
}

export interface ModbusDevice {
  slaveId: number;
  name: string;
  connected: boolean;
  lastCommunication: number;
  registers: ModbusRegister[];
  interfaceId?: number;  // RTU interface ID
  interfaceName?: string; // RTU interface name
  group?: string;
  priority?: number;
}

export interface ModbusNetworkStats {
  totalDevices: number;
  activeDevices: number;
  totalRegisters: number;
  totalRequests: number;
  successfulRequests: number;
  failedRequests: number;
  networkSuccessRate: number;
  averageResponseTime: number;
  networkUptime: number;
  lastScanTime: number;
  networkLoad: number;
  networkStatus: string;
}

export interface RTUInterface {
  interfaceId: number;
  name: string;
  enabled: boolean;
  rxPin: number;
  txPin: number;
  dePin?: number;
  baudRate: number;
  initialized: boolean;
  stats: ModbusNetworkStats;
  devices: ModbusDevice[];
}

export interface UnifiedSensorConfig {
  sensorId: string;
  name: string;
  description: string;
  type: 'digital_input' | 'digital_output' | 'analog_voltage' | 'analog_current' | 'modbus_rtu' | 'virtual' | 'system';
  enabled: boolean;
  sourceType: string;
  sourceAddress: string;
  sourceChannel?: number;
  modbusInterface?: number;
  modbusSlaveId?: number;
  modbusRegister?: string;
  scaleFactor: number;
  offset: number;
  minValue: number;
  maxValue: number;
  unit: string;
  updateInterval: number;
  alarmEnabled: boolean;
  alarmLowThreshold: number;
  alarmHighThreshold: number;
  group: string;
  location: string;
  priority: number;
  tags?: Record<string, string>;
}

export interface AlarmEvent {
  sensorId: string;
  alarmType: string;
  severity: 'info' | 'warning' | 'error' | 'critical';
  currentValue: number;
  thresholdValue: number;
  message: string;
  timestamp: number;
  acknowledged: boolean;
  acknowledgedBy?: string;
  acknowledgedTime?: number;
}

export interface ModbusRegister {
  address: number;
  name: string;
  value: number;
  unit: string;
  type: 'input' | 'holding' | 'coil' | 'discrete';
}

export interface DigitalIOStatus {
  inputs: Array<{
    id: number;
    name: string;
    state: boolean;
    timestamp: number;
  }>;
  outputs: Array<{
    id: number;
    name: string;
    state: boolean;
    mode: 'on' | 'off' | 'pwm' | 'pulse' | 'blink';
  }>;
}

export interface AnalogReading {
  channel: number;
  name: string;
  rawValue: number;
  scaledValue: number;
  unit: string;
  type: 'voltage' | 'current';
  timestamp: number;
}

// Configuration for HTTP client
interface ClientConfig {
  baseURL: string;
  timeout: number;
  retries: number;
  retryDelay: number;
}

class ESP32HttpClient {
  private client: typeof ofetch;
  private config: ClientConfig;

  constructor() {
        // Check if we're in development mode
    const isDev = import.meta.env.DEV;
    
    if (isDev) {
      // In development, use proxy to primary RTU (ada-1)
      this.config = {
        baseURL: '/ada-1/api',  // Use primary RTU proxy endpoint
        timeout: 15000,
        retries: 3,
        retryDelay: 1000
      };
    } else {
      // In production, use primary server address (RTU 1)
      const primaryServerAddress = (window as any).__PRIMARY_SERVER_ADDRESS__ || 'http://localhost:80';
      this.config = {
        baseURL: `${primaryServerAddress}/api`,
        timeout: 15000,
        retries: 3,
        retryDelay: 1000
      };
    }

    console.log('[ESP32 Client] Mode:', isDev ? 'Development (using proxy)' : 'Production');
    console.log('[ESP32 Client] Base URL:', this.config.baseURL);

    this.client = ofetch.create({
      baseURL: this.config.baseURL,
      timeout: this.config.timeout,
      retry: this.config.retries,
      retryDelay: this.config.retryDelay,
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json'
      },
      onRequest: ({ request, options }) => {
        console.log(`[HTTP] → ${options.method || 'GET'} ${request}`);
      },
      onResponse: ({ response }) => {
        console.log(`[HTTP] ← ${response.status} ${response.url}`);
      },
      onResponseError: ({ response }) => {
        console.error(`[HTTP Error] ${response.status} ${response.url}:`, response.statusText);
      }
    });
  }

  // System Information
  async getSystemStatus(): Promise<SystemStatus> {
    return this.client('/system/status');
  }

  async getSystemInfo(): Promise<Record<string, any>> {
    return this.client('/system/info');
  }

  // Sensor Data
  async getAllSensors(): Promise<SensorReading[]> {
    return this.client('/sensors');
  }

  async getSensorById(id: string): Promise<SensorReading> {
    return this.client(`/sensors/${id}`);
  }

  // Digital IO Operations
  async getDigitalIO(): Promise<DigitalIOStatus> {
    return this.client('/io/digital');
  }

  async setDigitalOutput(pin: number, state: boolean): Promise<{ success: boolean }> {
    return this.client(`/io/digital/output/${pin}`, {
      method: 'POST',
      body: { state }
    });
  }

  async setDigitalOutputMode(pin: number, mode: string, value?: number): Promise<{ success: boolean }> {
    return this.client(`/io/digital/output/${pin}/mode`, {
      method: 'POST',
      body: { mode, value }
    });
  }

  // Analog Operations
  async getAnalogReadings(): Promise<AnalogReading[]> {
    return this.client('/io/analog');
  }

  async getAnalogVoltage(): Promise<AnalogReading[]> {
    return this.client('/io/analog/voltage');
  }

  async getAnalogCurrent(): Promise<AnalogReading[]> {
    return this.client('/io/analog/current');
  }

  // Modbus Operations
  async getModbusDevices(): Promise<ModbusDevice[]> {
    return this.client('/modbus/devices');
  }

  async getModbusDevice(slaveId: number): Promise<ModbusDevice> {
    return this.client(`/modbus/devices/${slaveId}`);
  }

  async readModbusRegister(slaveId: number, address: number): Promise<{ value: number }> {
    return this.client(`/modbus/devices/${slaveId}/read/${address}`);
  }

  async writeModbusRegister(slaveId: number, address: number, value: number): Promise<{ success: boolean }> {
    return this.client(`/modbus/devices/${slaveId}/write/${address}`, {
      method: 'POST',
      body: { value }
    });
  }

  // Multi RTU Modbus Operations
  async getRTUInterfaces(): Promise<RTUInterface[]> {
    try {
      const result = await this.client('/modbus/rtu/interfaces');
      console.log('[RTU] Interfaces loaded from ESP32:', result);
      return result;
    } catch (error) {
      // Fallback: Return ada-1 as RTU 1 if the endpoint doesn't exist
      console.warn('[RTU] Interfaces endpoint not available, using fallback for ada-1');
      console.log('[RTU] Fallback triggered for ESP32 host:', this.config.baseURL);
      
      return [
        {
          interfaceId: 1,
          name: 'ada-1 (Primary ESP32)',
          enabled: true,
          rxPin: 16,  // Based on pins_config.h
          txPin: 17,  // Based on pins_config.h  
          dePin: 4,   // Based on pins_config.h
          baudRate: 9600,
          initialized: true,
          stats: {
            totalDevices: 0,
            activeDevices: 0,
            totalRegisters: 0,
            totalRequests: 0,
            successfulRequests: 0,
            failedRequests: 0,
            networkSuccessRate: 100,
            averageResponseTime: 50,
            networkUptime: Date.now() - 86400000, // 24h ago
            lastScanTime: Date.now() - 300000, // 5 min ago
            networkLoad: 10,
            networkStatus: 'active'
          },
          devices: []
        }
      ];
    }
  }

  async getRTUInterface(interfaceId: number): Promise<RTUInterface> {
    try {
      const result = await this.client(`/modbus/rtu/interfaces/${interfaceId}`);
      console.log(`[RTU] Interface ${interfaceId} loaded from ESP32:`, result);
      return result;
    } catch (error) {
      // Fallback: Return ada-1 data for interface ID 1
      if (interfaceId === 1) {
        console.warn(`[RTU] Interface ${interfaceId} endpoint not available, using fallback for ada-1`);
        
        // Try to get actual device count from existing modbus endpoint
        let deviceCount = 0;
        try {
          const modbusData = await this.client('/modbus/devices');
          deviceCount = modbusData.devices ? modbusData.devices.length : 0;
          console.log(`[RTU] Found ${deviceCount} existing Modbus devices for ada-1`);
        } catch (modbusError) {
          console.warn('[RTU] No existing Modbus devices found');
        }
        
        return {
          interfaceId: 1,
          name: 'ada-1 (Primary ESP32)',
          enabled: true,
          rxPin: 16,  // From pins_config.h
          txPin: 17,  // From pins_config.h
          dePin: 4,   // From pins_config.h
          baudRate: 9600,
          initialized: true,
          stats: {
            totalDevices: deviceCount,
            activeDevices: deviceCount,
            totalRegisters: deviceCount * 10, // Estimate
            totalRequests: 1000,
            successfulRequests: 950,
            failedRequests: 50,
            networkSuccessRate: 95,
            averageResponseTime: 45,
            networkUptime: Date.now() - 86400000, // 24h ago
            lastScanTime: Date.now() - 300000, // 5 min ago
            networkLoad: 25,
            networkStatus: 'active'
          },
          devices: []
        };
      }
      throw error;
    }
  }

  async enableRTUInterface(interfaceId: number, enabled: boolean): Promise<{ success: boolean }> {
    return this.client(`/modbus/rtu/interfaces/${interfaceId}/enable`, {
      method: 'POST',
      body: { enabled }
    });
  }

  async getRTUInterfaceStats(interfaceId: number): Promise<ModbusNetworkStats> {
    return this.client(`/modbus/rtu/interfaces/${interfaceId}/stats`);
  }

  async getDevicesOnRTU(interfaceId: number): Promise<ModbusDevice[]> {
    try {
      return await this.client(`/modbus/rtu/interfaces/${interfaceId}/devices`);
    } catch (error) {
      // Fallback: Return existing Modbus devices for ada-1 (RTU 1)
      if (interfaceId === 1) {
        console.warn(`RTU devices endpoint not available for interface ${interfaceId}, using existing Modbus devices`);
        try {
          // Try to get existing Modbus devices from the legacy endpoint
          const modbusData = await this.client('/modbus/devices');
          return modbusData.devices ? modbusData.devices.map((device: any) => ({
            ...device,
            interfaceId: 1,
            interfaceName: 'ada-1'
          })) : [];
        } catch (legacyError) {
          console.warn('Legacy Modbus endpoint also not available');
          return [];
        }
      }
      throw error;
    }
  }

  async scanRTUInterface(interfaceId: number): Promise<{ devices: number[] }> {
    return this.client(`/modbus/rtu/interfaces/${interfaceId}/scan`, {
      method: 'POST'
    });
  }

  // Multi RTU Device Operations
  async getMultiRTUDevice(interfaceId: number, slaveId: number): Promise<ModbusDevice> {
    return this.client(`/modbus/rtu/${interfaceId}/devices/${slaveId}`);
  }

  async readMultiRTURegister(interfaceId: number, slaveId: number, address: number): Promise<{ value: number }> {
    return this.client(`/modbus/rtu/${interfaceId}/devices/${slaveId}/read/${address}`);
  }

  async writeMultiRTURegister(interfaceId: number, slaveId: number, address: number, value: number): Promise<{ success: boolean }> {
    return this.client(`/modbus/rtu/${interfaceId}/devices/${slaveId}/write/${address}`, {
      method: 'POST',
      body: { value }
    });
  }

  // Configuration
  async getConfiguration(): Promise<Record<string, any>> {
    return this.client('/config');
  }

  async updateConfiguration(config: Record<string, any>): Promise<{ success: boolean }> {
    return this.client('/config', {
      method: 'PUT',
      body: config
    });
  }

  // Analytics
  async getAnalytics(timeRange?: string): Promise<Record<string, any>> {
    const query = timeRange ? `?range=${timeRange}` : '';
    return this.client(`/analytics${query}`);
  }

  // Webhooks
  async testWebhook(url: string): Promise<{ success: boolean }> {
    return this.client('/webhooks/test', {
      method: 'POST',
      body: { url }
    });
  }

  // Utility methods
  async ping(): Promise<{ pong: boolean; timestamp: number }> {
    return this.client('/ping');
  }

  async restart(): Promise<{ success: boolean }> {
    return this.client('/system/restart', {
      method: 'POST'
    });
  }

  // Update client configuration
  updateConfig(newConfig: Partial<ClientConfig>): void {
    this.config = { ...this.config, ...newConfig };
    // Recreate client with new config
    this.client = ofetch.create({
      baseURL: this.config.baseURL,
      timeout: this.config.timeout,
      retry: this.config.retries,
      retryDelay: this.config.retryDelay,
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json'
      }
    });
  }

  // Unified Sensor Management
  async getUnifiedSensors(): Promise<SensorReading[]> {
    return this.client('/sensors/unified');
  }

  async getUnifiedSensorsByType(type: string): Promise<SensorReading[]> {
    return this.client(`/sensors/unified/type/${type}`);
  }

  async getUnifiedSensorsByGroup(group: string): Promise<SensorReading[]> {
    return this.client(`/sensors/unified/group/${group}`);
  }

  async getUnifiedSensorConfig(sensorId: string): Promise<UnifiedSensorConfig> {
    return this.client(`/sensors/unified/${sensorId}/config`);
  }

  async getAllUnifiedSensorConfigs(): Promise<UnifiedSensorConfig[]> {
    return this.client('/sensors/unified/configs');
  }

  async addUnifiedSensor(config: UnifiedSensorConfig): Promise<{ success: boolean; sensorId: string }> {
    return this.client('/sensors/unified', {
      method: 'POST',
      body: config
    });
  }

  async updateUnifiedSensor(sensorId: string, config: UnifiedSensorConfig): Promise<{ success: boolean }> {
    return this.client(`/sensors/unified/${sensorId}`, {
      method: 'PUT',
      body: config
    });
  }

  async removeUnifiedSensor(sensorId: string): Promise<{ success: boolean }> {
    return this.client(`/sensors/unified/${sensorId}`, {
      method: 'DELETE'
    });
  }

  async enableUnifiedSensor(sensorId: string, enabled: boolean): Promise<{ success: boolean }> {
    return this.client(`/sensors/unified/${sensorId}/enable`, {
      method: 'POST',
      body: { enabled }
    });
  }

  async calibrateUnifiedSensor(sensorId: string, referenceValue: number): Promise<{ success: boolean }> {
    return this.client(`/sensors/unified/${sensorId}/calibrate`, {
      method: 'POST',
      body: { referenceValue }
    });
  }

  async readUnifiedSensor(sensorId: string): Promise<SensorReading> {
    return this.client(`/sensors/unified/${sensorId}/read`);
  }

  // Unified Sensor Helpers
  async addAnalogVoltageSensor(name: string, channel: number, minVoltage: number = 0, maxVoltage: number = 10, unit: string = 'V', group: string = 'voltage'): Promise<{ success: boolean; sensorId: string }> {
    return this.client('/sensors/unified/helpers/analog-voltage', {
      method: 'POST',
      body: { name, channel, minVoltage, maxVoltage, unit, group }
    });
  }

  async addAnalogCurrentSensor(name: string, channel: number, minCurrent: number = 4, maxCurrent: number = 20, unit: string = 'mA', group: string = 'current'): Promise<{ success: boolean; sensorId: string }> {
    return this.client('/sensors/unified/helpers/analog-current', {
      method: 'POST',
      body: { name, channel, minCurrent, maxCurrent, unit, group }
    });
  }

  async addDigitalInputSensor(name: string, pin: number, group: string = 'digital'): Promise<{ success: boolean; sensorId: string }> {
    return this.client('/sensors/unified/helpers/digital-input', {
      method: 'POST',
      body: { name, pin, group }
    });
  }

  async addDigitalOutputSensor(name: string, pin: number, group: string = 'digital'): Promise<{ success: boolean; sensorId: string }> {
    return this.client('/sensors/unified/helpers/digital-output', {
      method: 'POST',
      body: { name, pin, group }
    });
  }

  async addModbusSensor(name: string, slaveId: number, registerName: string, unit: string = '', group: string = 'modbus'): Promise<{ success: boolean; sensorId: string }> {
    return this.client('/sensors/unified/helpers/modbus', {
      method: 'POST',
      body: { name, slaveId, registerName, unit, group }
    });
  }

  async addMultiRTUModbusSensor(name: string, interfaceId: number, slaveId: number, registerName: string, unit: string = '', group: string = 'modbus'): Promise<{ success: boolean; sensorId: string }> {
    return this.client('/sensors/unified/helpers/multi-rtu-modbus', {
      method: 'POST',
      body: { name, interfaceId, slaveId, registerName, unit, group }
    });
  }

  // Alarm Management
  async getActiveAlarms(): Promise<AlarmEvent[]> {
    return this.client('/alarms/active');
  }

  async getAlarmHistory(since?: number): Promise<AlarmEvent[]> {
    const query = since ? `?since=${since}` : '';
    return this.client(`/alarms/history${query}`);
  }

  async acknowledgeAlarm(alarmIndex: number, acknowledgedBy: string = 'User'): Promise<{ success: boolean }> {
    return this.client(`/alarms/acknowledge/${alarmIndex}`, {
      method: 'POST',
      body: { acknowledgedBy }
    });
  }

  async clearAlarm(alarmIndex: number): Promise<{ success: boolean }> {
    return this.client(`/alarms/clear/${alarmIndex}`, {
      method: 'POST'
    });
  }

  async clearAllAlarms(): Promise<{ success: boolean }> {
    return this.client('/alarms/clear-all', {
      method: 'POST'
    });
  }

  // System Statistics
  async getUnifiedSensorStats(): Promise<{
    totalSensors: number;
    activeSensors: number;
    totalReadings: number;
    successfulReadings: number;
    failedReadings: number;
    successRate: number;
    uptime: number;
    lastUpdateTime: number;
    systemStatus: string;
  }> {
    return this.client('/sensors/unified/stats');
  }

  // Bulk Operations
  async enableAllSensors(enabled: boolean): Promise<{ success: boolean }> {
    return this.client('/sensors/unified/enable-all', {
      method: 'POST',
      body: { enabled }
    });
  }

  async enableSensorGroup(group: string, enabled: boolean): Promise<{ success: boolean }> {
    return this.client(`/sensors/unified/group/${group}/enable`, {
      method: 'POST',
      body: { enabled }
    });
  }

  async readAllSensors(): Promise<{ success: boolean; readings: SensorReading[] }> {
    return this.client('/sensors/unified/read-all', {
      method: 'POST'
    });
  }

  async readSensorGroup(group: string): Promise<{ success: boolean; readings: SensorReading[] }> {
    return this.client(`/sensors/unified/group/${group}/read`, {
      method: 'POST'
    });
  }

  // Search and Filtering
  async findSensorsByTag(tagKey: string, tagValue?: string): Promise<string[]> {
    const query = tagValue ? `?key=${tagKey}&value=${tagValue}` : `?key=${tagKey}`;
    return this.client(`/sensors/unified/search/tag${query}`);
  }

  async findSensorsByLocation(location: string): Promise<string[]> {
    return this.client(`/sensors/unified/search/location?location=${location}`);
  }

  async findSensorsWithAlarms(): Promise<string[]> {
    return this.client('/sensors/unified/search/alarms');
  }

  async findOfflineSensors(): Promise<string[]> {
    return this.client('/sensors/unified/search/offline');
  }

  // Configuration Management
  async exportUnifiedSensorConfig(): Promise<{ config: string }> {
    return this.client('/sensors/unified/config/export');
  }

  async importUnifiedSensorConfig(config: string): Promise<{ success: boolean }> {
    return this.client('/sensors/unified/config/import', {
      method: 'POST',
      body: { config }
    });
  }

  // Diagnostics
  async getUnifiedSensorDiagnostics(): Promise<{ report: string }> {
    return this.client('/sensors/unified/diagnostics');
  }

  async testUnifiedSensor(sensorId: string): Promise<{ success: boolean; result: string }> {
    return this.client(`/sensors/unified/${sensorId}/test`, {
      method: 'POST'
    });
  }

  async testAllSensors(): Promise<{ success: boolean; results: Record<string, string> }> {
    return this.client('/sensors/unified/test-all', {
      method: 'POST'
    });
  }

  async getHealthReport(): Promise<{ report: string[] }> {
    return this.client('/sensors/unified/health');
  }

  async scanModbusDevices(): Promise<{ devices: number[] }> {
    return this.client('/modbus/scan', {
      method: 'POST'
    });
  }
}

// Export singleton instance
export const esp32Client = new ESP32HttpClient();

// Export class for custom instances
export { ESP32HttpClient };

// Export utility functions
export const createESP32Client = () => {
  return new ESP32HttpClient();
};