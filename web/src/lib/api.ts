import { ofetch } from 'ofetch';
import { z } from 'zod';

// Base API configuration
const api = ofetch.create({
  baseURL: '/api',
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
  status: z.string(),
  uptime: z.number().optional(),
  firmware: z.string().optional(),
  device: z.string().optional(),
  timestamp: z.number().optional(),
  heap: z.number().optional(),
  wifi: z.object({
    ssid: z.string(),
    rssi: z.number(),
    ip: z.string(),
  }).optional(),
});

export const AnalogVoltageSchema = z.object({
  sensors: z.array(z.object({
    id: z.number(),
    voltage: z.number(),
    raw: z.number(),
    status: z.string(),
    timestamp: z.number(),
  })),
  metadata: z.object({
    sampling_rate: z.number(),
    resolution: z.number(),
    reference_voltage: z.number(),
  }).optional(),
});

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

  // Sensor data
  async getAnalogVoltage(): Promise<AnalogVoltageResponse> {
    const data = await api('/analog-voltage');
    return AnalogVoltageSchema.parse(data);
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

  // System control
  async restart() {
    return api('/restart', { method: 'POST' });
  }

  async resetWifi() {
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

  // Diagnostics
  async getDiagnosticsStatus() {
    return api('/diagnostics/status');
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