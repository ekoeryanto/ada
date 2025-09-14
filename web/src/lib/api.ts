import { ofetch } from 'ofetch';
import { z } from 'zod';

// Base API configuration
const isProduction = typeof window !== 'undefined' && window.location.hostname !== 'localhost';
const ESP32_IP = '192.168.111.34'; // ESP32 IP address

const api = ofetch.create({
  baseURL: isProduction ? '/api' : `http://${ESP32_IP}/api`,
  retry: 3,
  retryDelay: 500,
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
  onRequestError({ error }) {
    console.error('API Request Error:', error);
  },
  onResponseError({ response }) {
    console.error('API Response Error:', response.status, response.statusText);
  },
});

// Zod schemas for API validation
export const StatusSchema = z.object({
  project: z.string(),
  version: z.string(),
  author: z.string(),
  status: z.string(),
  uptime: z.string(),
  freeHeap: z.number(),
  chipId: z.number(),
  wifi: z.object({
    connected: z.boolean(),
    ssid: z.string(),
    ip: z.string(),
    rssi: z.number(),
  }),
  ota: z.object({
    enabled: z.boolean(),
    status: z.string(),
    url: z.string(),
  }),
  sd: z.object({
    mounted: z.boolean(),
  }),
  ntp: z.object({
    initialized: z.boolean(),
    synced: z.boolean(),
    last_sync: z.number(),
    current_time: z.string(),
    rtc_available: z.boolean(),
  }),
  analog_voltage: z.object({
    initialized: z.boolean(),
    total_readings: z.number(),
    alarm_status: z.string(),
    has_errors: z.boolean(),
    sensors: z.record(z.string(), z.object({
      location: z.string(),
      enabled: z.boolean(),
      value: z.number(),
      unit: z.string(),
      voltage: z.number(),
      raw_voltage: z.number(),
      status: z.string(),
      health_score: z.number(),
      is_dead: z.boolean(),
      is_stuck: z.boolean(),
      is_calibrated: z.boolean(),
      low_alarm: z.boolean(),
      high_alarm: z.boolean(),
      errors: z.number(),
    })),
  }).optional(),
});

export const AnalogCurrentSchema = z.object({
  initialized: z.boolean(),
  total_readings: z.number(),
  alarm_status: z.string(),
  has_errors: z.boolean(),
  sensors: z.record(z.string(), z.object({
    location: z.string(),
    enabled: z.boolean(),
    value: z.number(),
    unit: z.string(),
    current: z.number(),
    voltage: z.number(),
    raw_adc: z.number(),
    status: z.string(),
    valid: z.boolean(),
    timestamp: z.number(),
    loop_resistance: z.number(),
    signal_quality: z.number(),
    low_alarm: z.boolean(),
    high_alarm: z.boolean(),
    errors: z.number(),
  })),
});

// UI compatible response type
export interface AnalogCurrentUIResponse {
  initialized: boolean;
  total_readings: number;
  alarm_status: string;
  has_errors: boolean;
  sensors: Record<string, {
    id: string;
    location: string;
    enabled: boolean;
    value: number;
    unit: string;
    current: number;
    voltage: number;
    raw_adc: number;
    status: string;
    valid: boolean;
    timestamp: number;
    loop_resistance: number;
    signal_quality: number;
    low_alarm: boolean;
    high_alarm: boolean;
    errors: number;
  }>;
  metadata?: {
    sampling_rate: number;
    resolution: number;
    loop_power: string;
  };
}

export const DigitalIOSchema = z.object({
  initialized: z.boolean(),
  total_inputs: z.number(),
  total_outputs: z.number(),
  inputs: z.record(z.string(), z.object({
    name: z.string(),
    state: z.union([z.string(), z.boolean()]), // Can be "HIGH"/"LOW" or boolean
    valid: z.boolean().optional(),
    pulse_count: z.number().optional(),
    total_pulses: z.number().optional(),
    state_time: z.number().optional(),
    last_change: z.number(),
    alarm_active: z.boolean().optional(),
    timestamp: z.number(),
  })),
  outputs: z.record(z.string(), z.object({
    name: z.string(),
    state: z.union([z.number(), z.boolean()]), // Can be 0/1 or boolean
    physical_state: z.boolean().optional(),
    duty_cycle: z.number().optional(),
    operations: z.number().optional(),
    last_operation: z.number().optional(),
    current_state: z.number().optional(),
  })),
});

// Converted response type for UI compatibility
export interface DigitalIOUIResponse {
  initialized: boolean;
  total_inputs: number;
  total_outputs: number;
  inputs: Record<string, {
    id: string;
    pin: number;
    name: string;
    state: boolean;
    pullup: boolean;
    last_change: number;
    timestamp: number;
    pulse_count: number;
    alarm_active: boolean;
  }>;
  outputs: Record<string, {
    id: string;
    pin: number;
    name: string;
    state: boolean;
    mode: string;
    timestamp: number;
    last_change: number;
    operations: number;
  }>;
}

export const DiagnosticsSchema = z.object({
  status: z.string(),
  alerts: z.array(z.object({
    id: z.string(),
    level: z.string(),
    message: z.string(),
    timestamp: z.number(),
    component: z.string(),
  })),
  health: z.object({
    overall: z.string(),
    components: z.array(z.object({
      name: z.string(),
      status: z.string(),
      details: z.string().optional(),
    })),
  }),
});

export const WebhookSchema = z.object({
  webhooks: z.array(z.object({
    id: z.string(),
    url: z.string(),
    events: z.array(z.string()),
    enabled: z.boolean(),
    created_at: z.number(),
  })),
  statistics: z.object({
    total_sent: z.number(),
    success_rate: z.number(),
    queue_size: z.number(),
  }).optional(),
});

export const ModbusSchema = z.object({
  initialized: z.boolean(),
  total_devices: z.number(),
  connected_devices: z.number(),
  network: z.object({
    success_rate: z.number(),
    avg_response_time: z.number(),
    total_requests: z.number(),
    failed_requests: z.number(),
    active_devices: z.number(),
  }),
  devices: z.array(z.object({
    id: z.number(),
    address: z.number(),
    name: z.string(),
    type: z.string(),
    status: z.string(),
    last_communication: z.number(),
    error_count: z.number(),
    enabled: z.boolean(),
    registers_read: z.number(),
    registers_written: z.number(),
    response_time: z.number(),
  })).default([]),
});

// UI compatible response type (devices already array)
export interface ModbusUIResponse {
  initialized: boolean;
  total_devices: number;
  connected_devices: number;
  network: {
    success_rate: number;
    avg_response_time: number;
    total_requests: number;
    failed_requests: number;
    active_devices: number;
  };
  devices: Array<{
    id: number;
    address: number;
    name: string;
    type: string;
    status: string;
    last_communication: number;
    error_count: number;
    enabled: boolean;
    registers_read: number;
    registers_written: number;
    response_time: number;
  }>;
  statistics?: {
    total_requests: number;
    success_rate: number;
    error_count: number;
  };
}

export const AnalogVoltageSchema = z.object({
  initialized: z.boolean(),
  total_readings: z.number(),
  alarm_status: z.string(),
  has_errors: z.boolean(),
  sensors: z.record(z.string(), z.object({
    location: z.string(),
    enabled: z.boolean(),
    value: z.number(),
    unit: z.string(),
    voltage: z.number(),
    raw_adc: z.number(),
    raw_voltage: z.number(),
    status: z.string(),
    valid: z.boolean(),
    timestamp: z.number(),
    low_alarm: z.boolean(),
    high_alarm: z.boolean(),
    errors: z.number(),
  })),
});

// UI compatible response type
export interface AnalogVoltageUIResponse {
  initialized: boolean;
  total_readings: number;
  alarm_status: string;
  has_errors: boolean;
  sensors: Record<string, {
    id: string;
    location: string;
    enabled: boolean;
    value: number;
    unit: string;
    voltage: number;
    raw_adc: number;
    raw_voltage: number;
    status: string;
    valid: boolean;
    timestamp: number;
    low_alarm: boolean;
    high_alarm: boolean;
    errors: number;
  }>;
  metadata?: {
    sampling_rate: number;
    resolution: number;
    reference_voltage: number;
  };
}

export const ConfigSchema = z.object({
  sensors: z.array(z.object({
    id: z.number(),
    enabled: z.boolean(),
    calibration: z.object({
      offset: z.number(),
      scale: z.number(),
    }).optional(),
  })),
  system: z.object({
    sampling_interval: z.number(),
    data_retention: z.number(),
  }).optional(),
});

export const SensorHealthSchema = z.object({
  sensors: z.array(z.object({
    id: z.number(),
    health: z.string(),
    error_count: z.number(),
    last_reading: z.number(),
    status: z.string(),
  })),
});

export const AnalyticsSchema = z.object({
  summary: z.object({
    total_readings: z.number(),
    average_voltage: z.number(),
    min_voltage: z.number(),
    max_voltage: z.number(),
    uptime: z.number(),
  }),
  trends: z.array(z.object({
    timestamp: z.number(),
    value: z.number(),
  })).optional(),
});

// Type definitions
export type StatusResponse = z.infer<typeof StatusSchema>;
export type AnalogVoltageResponse = z.infer<typeof AnalogVoltageSchema>;
export type AnalogCurrentResponse = z.infer<typeof AnalogCurrentSchema>;
export type DigitalIOResponse = z.infer<typeof DigitalIOSchema>;
export type DiagnosticsResponse = z.infer<typeof DiagnosticsSchema>;
export type WebhookResponse = z.infer<typeof WebhookSchema>;
export type ModbusResponse = z.infer<typeof ModbusSchema>;
export type ConfigResponse = z.infer<typeof ConfigSchema>;
export type SensorHealthResponse = z.infer<typeof SensorHealthSchema>;
export type AnalyticsResponse = z.infer<typeof AnalyticsSchema>;

// API client class
export class AdaApiClient {
  // System status
  async getStatus(): Promise<StatusResponse> {
    const data = await api('/status');
    return StatusSchema.parse(data);
  }

  async getHealth() {
    return api('/health');
  }

  // Analog Voltage Sensors (0-10V)
  async getAnalogVoltage(): Promise<AnalogVoltageUIResponse> {
    const data = await api('/analog-voltage');
    const parsed = AnalogVoltageSchema.parse(data);
    
    // Keep object format with hardware keys (ai1, ai2, ai3) for UI compatibility
    const sensorsObject = Object.fromEntries(
      Object.entries(parsed.sensors).map(([key, sensor]) => [
        key, // Keep original key (ai1, ai2, ai3)
        {
          id: key, // Use key as id instead of parsing to int
          location: sensor.location,
          enabled: sensor.enabled,
          value: sensor.value,
          unit: sensor.unit,
          voltage: sensor.voltage,
          raw_adc: sensor.raw_adc,
          raw_voltage: sensor.raw_voltage,
          status: sensor.status,
          valid: sensor.valid,
          timestamp: sensor.timestamp,
          low_alarm: sensor.low_alarm,
          high_alarm: sensor.high_alarm,
          errors: sensor.errors,
        }
      ])
    );

    return {
      ...parsed,
      sensors: sensorsObject, // Return object, not array
    };
  }

  // Analog Current Sensors (4-20mA) - NEW!
  async getAnalogCurrent(): Promise<AnalogCurrentUIResponse> {
    const data = await api('/analog-current');
    const parsed = AnalogCurrentSchema.parse(data);
    
    // Keep object format with hardware keys (aci1, aci2) for UI compatibility
    const sensorsObject = Object.fromEntries(
      Object.entries(parsed.sensors).map(([key, sensor]) => [
        key, // Keep original key (aci1, aci2)
        {
          id: key, // Use key as id instead of parsing to int
          location: sensor.location,
          enabled: sensor.enabled,
          value: sensor.value,
          unit: sensor.unit,
          current: sensor.current,
          voltage: sensor.voltage,
          raw_adc: sensor.raw_adc,
          status: sensor.status,
          valid: sensor.valid,
          timestamp: sensor.timestamp,
          loop_resistance: sensor.loop_resistance,
          signal_quality: sensor.signal_quality,
          low_alarm: sensor.low_alarm,
          high_alarm: sensor.high_alarm,
          errors: sensor.errors,
        }
      ])
    );

    return {
      ...parsed,
      sensors: sensorsObject, // Return object, not array
    };
  }

  async getAnalogCurrentHealth() {
    return api('/analog-current/health');
  }

  async getAnalogCurrentInfo() {
    return api('/analog-current/info');
  }

  async getAnalogCurrentDiagnostics() {
    return api('/analog-current/diagnostics');
  }

  // Digital I/O - NEW!
  async getDigitalIO(): Promise<DigitalIOUIResponse> {
    const data = await api('/digital-io');
    const parsed = DigitalIOSchema.parse(data);
    
    // Keep object format with hardware keys (di1, di2, di3, di4 and do1, do2, do3, do4) for UI compatibility
    const inputsObject = Object.fromEntries(
      Object.entries(parsed.inputs).map(([key, input]) => [
        key, // Keep original key (di1, di2, di3, di4)
        {
          id: key, // Use key as id
          pin: parseInt(key.replace('di', '')) + 24, // di1->25, di2->26, etc (approximate from pins_config.h)
          name: input.name,
          state: input.state === "HIGH" || input.state === true,
          pullup: true, // Default assumption
          last_change: input.last_change || 0,
          timestamp: input.timestamp,
          pulse_count: input.pulse_count || 0,
          alarm_active: input.alarm_active || false,
        }
      ])
    );
    
    const outputsObject = Object.fromEntries(
      Object.entries(parsed.outputs).map(([key, output]) => [
        key, // Keep original key (do1, do2, do3, do4)
        {
          id: key, // Use key as id
          pin: parseInt(key.replace('do', '')) + 31, // do1->32, do2->33, etc (approximate from pins_config.h)
          name: output.name,
          state: output.state === 1 || output.state === true,
          mode: "output",
          timestamp: Date.now() / 1000, // Current timestamp in seconds
          last_change: output.last_operation || 0,
          operations: output.operations || 0,
        }
      ])
    );
    
    return {
      initialized: parsed.initialized,
      total_inputs: parsed.total_inputs,
      total_outputs: parsed.total_outputs,
      inputs: inputsObject, // Return object, not array
      outputs: outputsObject, // Return object, not array
    };
  }

  async getDigitalInputs() {
    return api('/digital-io/inputs');
  }

  async getDigitalOutputs() {
    return api('/digital-io/outputs');
  }

  async setDigitalOutput(outputId: number | string, state: boolean) {
    const formData = new URLSearchParams();
    // Convert string id (like "do1") to number by extracting digit
    const numericId = typeof outputId === 'string' ? 
      parseInt(outputId.replace(/[^\d]/g, '')) : outputId;
    formData.append('output', numericId.toString());
    formData.append('state', state ? '1' : '0');
    
    return api('/digital-io/output', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: formData.toString(),
    });
  }

  async getSensorHealth(): Promise<SensorHealthResponse> {
    const data = await api('/analog-voltage/health');
    return SensorHealthSchema.parse(data);
  }

  async getSensorInfo() {
    return api('/analog-voltage/info');
  }

  // Configuration
  async getConfig(): Promise<ConfigResponse> {
    const data = await api('/config');
    return ConfigSchema.parse(data);
  }

  // Settings
  async getSettings() {
    return api('/settings');
  }

  async saveSettings(settings: any) {
    return api('/settings', {
      method: 'POST',
      body: settings,
    });
  }

  // Calibration
  async calibrateSensor(calibrationData: { sensor: number; offset?: number; scale?: number }) {
    return api('/analog-voltage/calibrate', {
      method: 'POST',
      body: calibrationData,
    });
  }

  async resetCalibration(sensor: number) {
    return api('/analog-voltage/reset-calibration', {
      method: 'POST',
      body: { sensor },
    });
  }

  // Analytics
  async getAnalytics(): Promise<AnalyticsResponse> {
    const data = await api('/analytics/summary');
    return AnalyticsSchema.parse(data);
  }

  async getAnalyticsStatistics() {
    return api('/analytics/statistics');
  }

  async getAnalyticsTrends() {
    return api('/analytics/trends');
  }

  async getAnalyticsPrediction() {
    return api('/analytics/prediction');
  }

  // System control - ENHANCED!
  async restart() {
    return api('/restart', { method: 'POST' });
  }

  async resetWifi() {
    return api('/reset', { method: 'POST' });
  }

  async resetSystem() {
    return api('/reset', { method: 'POST' });
  }

  // SD Card
  async getSDStatus() {
    return api('/sd/status');
  }

  async testSD() {
    return api('/sd/test', { method: 'POST' });
  }

  async getSDFiles() {
    return api('/sd/files');
  }

  // Simulation
  async enableSimulation(enabled: boolean) {
    return api('/simulation/enable', {
      method: 'POST',
      body: { enabled },
    });
  }

  async configureSensorSimulation(config: { sensor: number; type: string; params: Record<string, any> }) {
    return api('/simulation/sensor', {
      method: 'POST',
      body: config,
    });
  }

  async getSimulationStatus() {
    return api('/simulation/status');
  }

  // Diagnostics - ENHANCED!
  async getDiagnosticsStatus(): Promise<DiagnosticsResponse> {
    const data = await api('/diagnostics/status');
    return DiagnosticsSchema.parse(data);
  }

  async getAllDiagnostics() {
    return api('/diagnostics/all');
  }

  async getDiagnosticsAlerts() {
    return api('/diagnostics/alerts');
  }

  async getDiagnosticsHealth() {
    return api('/diagnostics/health');
  }

  async clearDiagnosticsAlerts() {
    return api('/diagnostics/clear-alerts', { method: 'POST' });
  }

  // Webhooks - ENHANCED!
  async getWebhooks(): Promise<WebhookResponse> {
    const data = await api('/webhooks');
    return WebhookSchema.parse(data);
  }

  async createWebhook(webhook: { 
    name: string; 
    url: string; 
    method?: string;
    timeout?: number;
    events: string[]; 
    enabled?: boolean;
    headers?: any;
    template?: string;
    description?: string;
  }) {
    return api('/webhooks', {
      method: 'POST',
      body: webhook,
    });
  }

  async updateWebhook(id: string, webhook: { 
    name: string; 
    url: string; 
    method?: string;
    timeout?: number;
    events: string[]; 
    enabled?: boolean;
    headers?: any;
    template?: string;
    description?: string;
  }) {
    return api(`/webhooks/${id}`, {
      method: 'PUT',
      body: webhook,
    });
  }

  async deleteWebhook(id: string) {
    return api(`/webhooks/${id}`, {
      method: 'DELETE',
    });
  }

  async getWebhookStatus() {
    return api('/webhooks/status');
  }

  async getWebhookStatistics() {
    return api('/webhooks/statistics');
  }

  async getWebhookQueue() {
    return api('/webhooks/queue');
  }

  async clearWebhookQueue() {
    return api('/webhooks/queue/clear', { method: 'POST' });
  }

  async retryWebhooks() {
    return api('/webhooks/queue/retry', { method: 'POST' });
  }

  async testWebhook(data: { url: string; method?: string; headers?: any; payload?: any }) {
    return api('/webhooks/test', {
      method: 'POST',
      body: data,
    });
  }

  // Modbus Communication - NEW!
  async getModbusDevices(): Promise<ModbusUIResponse> {
    const data = await api('/modbus/devices');
    const parsed = ModbusSchema.parse(data);
    
    // Return as-is since devices is already an array
    return {
      ...parsed,
      devices: parsed.devices,
      statistics: {
        total_requests: parsed.network.total_requests,
        success_rate: parsed.network.success_rate,
        error_count: parsed.network.failed_requests,
      },
    };
  }

  async addModbusDevice(device: { address: number; name: string; type: string }) {
    return api('/modbus/devices', {
      method: 'POST',
      body: device,
    });
  }

  async getModbusDevice(deviceId: string) {
    return api(`/modbus/devices/${deviceId}`);
  }

  async removeModbusDevice(deviceId: string) {
    return api(`/modbus/devices/${deviceId}`, { method: 'DELETE' });
  }

  async readModbus(data: { device: number; register: number; count?: number }) {
    return api('/modbus/read', {
      method: 'POST',
      body: data,
    });
  }

  async writeModbus(data: { device: number; register: number; value: number }) {
    return api('/modbus/write', {
      method: 'POST',
      body: data,
    });
  }

  async discoverModbusDevices() {
    return api('/modbus/discover', { method: 'POST' });
  }

  async getModbusStats() {
    return api('/modbus/stats');
  }

  // Generic method for endpoints that might not be implemented
  async safeGet(endpoint: string, fallbackValue: any = null) {
    try {
      return await api(endpoint);
    } catch (error) {
      console.warn(`Endpoint ${endpoint} not available:`, error);
      return fallbackValue;
    }
  }

  async safePost(endpoint: string, body: any = {}, fallbackValue: any = null) {
    try {
      return await api(endpoint, { method: 'POST', body });
    } catch (error) {
      console.warn(`Endpoint ${endpoint} not available:`, error);
      return fallbackValue;
    }
  }
}

// Export singleton instance
export const adaApi = new AdaApiClient();