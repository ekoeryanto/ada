import { esp32Client, type SensorReading, type SystemStatus, type ModbusDevice, type RTUInterface, type UnifiedSensorConfig, type AlarmEvent, type ModbusNetworkStats } from './esp32-client';
import { adaApi } from './api';

// Re-export types for convenience
export type { 
  SensorReading, 
  SystemStatus, 
  ModbusDevice, 
  RTUInterface,
  UnifiedSensorConfig,
  AlarmEvent,
  ModbusNetworkStats
} from './esp32-client';

// Enhanced API wrapper that combines legacy and new unified functionality
export class EnhancedESP32API {
  // Legacy analog voltage methods (keeping existing functionality)
  static async getAnalogVoltage() {
    return adaApi.getAnalogVoltage();
  }

  static async getSensorHealth() {
    return adaApi.getSensorHealth();
  }

  static async getSensorInfo() {
    return adaApi.getSensorInfo();
  }

  static async getConfig() {
    return adaApi.getConfig();
  }

  static async calibrateSensor(calibrationData: { sensor: number; offset?: number; scale?: number }) {
    return adaApi.calibrateSensor(calibrationData);
  }

  static async resetCalibration(sensor: number) {
    return adaApi.resetCalibration(sensor);
  }

  static async getAnalytics() {
    return adaApi.getAnalytics();
  }

  static async getStatus() {
    return adaApi.getStatus();
  }

  // System methods
  static async getSystemStatus(): Promise<SystemStatus> {
    return esp32Client.getSystemStatus();
  }

  static async getSystemInfo() {
    return esp32Client.getSystemInfo();
  }

  static async restart() {
    return esp32Client.restart();
  }

  static async ping() {
    return esp32Client.ping();
  }

  // Unified Sensor Management - NEW
  static async getUnifiedSensors(): Promise<SensorReading[]> {
    return esp32Client.getUnifiedSensors();
  }

  static async getUnifiedSensorsByType(type: string): Promise<SensorReading[]> {
    return esp32Client.getUnifiedSensorsByType(type);
  }

  static async getUnifiedSensorsByGroup(group: string): Promise<SensorReading[]> {
    return esp32Client.getUnifiedSensorsByGroup(group);
  }

  static async getUnifiedSensorConfig(sensorId: string): Promise<UnifiedSensorConfig> {
    return esp32Client.getUnifiedSensorConfig(sensorId);
  }

  static async getAllUnifiedSensorConfigs(): Promise<UnifiedSensorConfig[]> {
    return esp32Client.getAllUnifiedSensorConfigs();
  }

  static async addUnifiedSensor(config: UnifiedSensorConfig) {
    return esp32Client.addUnifiedSensor(config);
  }

  static async updateUnifiedSensor(sensorId: string, config: UnifiedSensorConfig) {
    return esp32Client.updateUnifiedSensor(sensorId, config);
  }

  static async removeUnifiedSensor(sensorId: string) {
    return esp32Client.removeUnifiedSensor(sensorId);
  }

  static async enableUnifiedSensor(sensorId: string, enabled: boolean) {
    return esp32Client.enableUnifiedSensor(sensorId, enabled);
  }

  static async calibrateUnifiedSensor(sensorId: string, referenceValue: number) {
    return esp32Client.calibrateUnifiedSensor(sensorId, referenceValue);
  }

  static async readUnifiedSensor(sensorId: string): Promise<SensorReading> {
    return esp32Client.readUnifiedSensor(sensorId);
  }

  // Multi RTU Modbus - NEW
  static async getRTUInterfaces(): Promise<RTUInterface[]> {
    return esp32Client.getRTUInterfaces();
  }

  static async getRTUInterface(interfaceId: number): Promise<RTUInterface> {
    return esp32Client.getRTUInterface(interfaceId);
  }

  static async enableRTUInterface(interfaceId: number, enabled: boolean) {
    return esp32Client.enableRTUInterface(interfaceId, enabled);
  }

  static async getRTUInterfaceStats(interfaceId: number): Promise<ModbusNetworkStats> {
    return esp32Client.getRTUInterfaceStats(interfaceId);
  }

  static async getDevicesOnRTU(interfaceId: number): Promise<ModbusDevice[]> {
    return esp32Client.getDevicesOnRTU(interfaceId);
  }

  static async scanRTUInterface(interfaceId: number) {
    return esp32Client.scanRTUInterface(interfaceId);
  }

  static async getMultiRTUDevice(interfaceId: number, slaveId: number): Promise<ModbusDevice> {
    return esp32Client.getMultiRTUDevice(interfaceId, slaveId);
  }

  static async readMultiRTURegister(interfaceId: number, slaveId: number, address: number) {
    return esp32Client.readMultiRTURegister(interfaceId, slaveId, address);
  }

  static async writeMultiRTURegister(interfaceId: number, slaveId: number, address: number, value: number) {
    return esp32Client.writeMultiRTURegister(interfaceId, slaveId, address, value);
  }

  // Digital IO
  static async getDigitalIO() {
    return esp32Client.getDigitalIO();
  }

  static async setDigitalOutput(pin: number, state: boolean) {
    return esp32Client.setDigitalOutput(pin, state);
  }

  static async setDigitalOutputMode(pin: number, mode: string, value?: number) {
    return esp32Client.setDigitalOutputMode(pin, mode, value);
  }

  // Analog Operations
  static async getAnalogReadings() {
    return esp32Client.getAnalogReadings();
  }

  static async getAnalogCurrent() {
    return esp32Client.getAnalogCurrent();
  }

  // Legacy Modbus (single RTU)
  static async getModbusDevices() {
    return esp32Client.getModbusDevices();
  }

  static async getModbusDevice(slaveId: number) {
    return esp32Client.getModbusDevice(slaveId);
  }

  static async scanModbusDevices() {
    return esp32Client.scanModbusDevices();
  }

  // Sensor Creation Helpers - NEW
  static async addAnalogVoltageSensor(name: string, channel: number, minVoltage = 0, maxVoltage = 10, unit = 'V', group = 'voltage') {
    return esp32Client.addAnalogVoltageSensor(name, channel, minVoltage, maxVoltage, unit, group);
  }

  static async addAnalogCurrentSensor(name: string, channel: number, minCurrent = 4, maxCurrent = 20, unit = 'mA', group = 'current') {
    return esp32Client.addAnalogCurrentSensor(name, channel, minCurrent, maxCurrent, unit, group);
  }

  static async addDigitalInputSensor(name: string, pin: number, group = 'digital') {
    return esp32Client.addDigitalInputSensor(name, pin, group);
  }

  static async addDigitalOutputSensor(name: string, pin: number, group = 'digital') {
    return esp32Client.addDigitalOutputSensor(name, pin, group);
  }

  static async addModbusSensor(name: string, slaveId: number, registerName: string, unit = '', group = 'modbus') {
    return esp32Client.addModbusSensor(name, slaveId, registerName, unit, group);
  }

  static async addMultiRTUModbusSensor(name: string, interfaceId: number, slaveId: number, registerName: string, unit = '', group = 'modbus') {
    return esp32Client.addMultiRTUModbusSensor(name, interfaceId, slaveId, registerName, unit, group);
  }

  // Alarm Management - NEW
  static async getActiveAlarms(): Promise<AlarmEvent[]> {
    return esp32Client.getActiveAlarms();
  }

  static async getAlarmHistory(since?: number): Promise<AlarmEvent[]> {
    return esp32Client.getAlarmHistory(since);
  }

  static async acknowledgeAlarm(alarmIndex: number, acknowledgedBy = 'User') {
    return esp32Client.acknowledgeAlarm(alarmIndex, acknowledgedBy);
  }

  static async clearAlarm(alarmIndex: number) {
    return esp32Client.clearAlarm(alarmIndex);
  }

  static async clearAllAlarms() {
    return esp32Client.clearAllAlarms();
  }

  // Statistics - NEW
  static async getUnifiedSensorStats() {
    return esp32Client.getUnifiedSensorStats();
  }

  // Bulk Operations - NEW
  static async enableAllSensors(enabled: boolean) {
    return esp32Client.enableAllSensors(enabled);
  }

  static async enableSensorGroup(group: string, enabled: boolean) {
    return esp32Client.enableSensorGroup(group, enabled);
  }

  static async readAllSensors() {
    return esp32Client.readAllSensors();
  }

  static async readSensorGroup(group: string) {
    return esp32Client.readSensorGroup(group);
  }

  // Search and Filtering - NEW
  static async findSensorsByTag(tagKey: string, tagValue?: string) {
    return esp32Client.findSensorsByTag(tagKey, tagValue);
  }

  static async findSensorsByLocation(location: string) {
    return esp32Client.findSensorsByLocation(location);
  }

  static async findSensorsWithAlarms() {
    return esp32Client.findSensorsWithAlarms();
  }

  static async findOfflineSensors() {
    return esp32Client.findOfflineSensors();
  }

  // Diagnostics - NEW
  static async getUnifiedSensorDiagnostics() {
    return esp32Client.getUnifiedSensorDiagnostics();
  }

  static async testUnifiedSensor(sensorId: string) {
    return esp32Client.testUnifiedSensor(sensorId);
  }

  static async testAllSensors() {
    return esp32Client.testAllSensors();
  }

  static async getHealthReport() {
    return esp32Client.getHealthReport();
  }

  // Configuration Management - NEW
  static async exportUnifiedSensorConfig() {
    return esp32Client.exportUnifiedSensorConfig();
  }

  static async importUnifiedSensorConfig(config: string) {
    return esp32Client.importUnifiedSensorConfig(config);
  }

  // Legacy compatibility methods
  static async getAllSensors(): Promise<SensorReading[]> {
    // Try to get unified sensors, fallback to legacy if needed
    try {
      return await esp32Client.getUnifiedSensors();
    } catch (error) {
      console.warn('Unified sensors not available, using legacy method');
      // Could implement fallback to legacy sensors here
      throw error;
    }
  }

  static async getSensor(id: string): Promise<SensorReading> {
    return esp32Client.readUnifiedSensor(id);
  }
}

// Direct client export for modern usage
export { esp32Client as client };

// Export enhanced API as default
export default EnhancedESP32API;