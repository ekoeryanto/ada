import { atom } from 'xoid';
import { adaApi, type StatusResponse, type AnalogVoltageResponse, type SensorHealthResponse, type AnalyticsResponse } from './api';

// Types for store state
interface SensorState {
  status: StatusResponse | null;
  voltage: AnalogVoltageResponse | null;
  health: SensorHealthResponse | null;
  lastUpdate: number;
  isLoading: boolean;
  error: string | null;
}

interface SystemState {
  status: StatusResponse | null;
  lastUpdate: number;
  isLoading: boolean;
  error: string | null;
}

interface ConfigState {
  config: any;
  lastUpdate: number;
  isLoading: boolean;
  error: string | null;
}

interface AnalyticsState {
  data: AnalyticsResponse | null;
  lastUpdate: number;
  isLoading: boolean;
  error: string | null;
}

interface Notification {
  id: string;
  type: 'success' | 'info' | 'warning' | 'error';
  title: string;
  message: string;
  timestamp: number;
  read: boolean;
}

interface UIState {
  theme: 'light' | 'dark';
  sidebarOpen: boolean;
  activeTab: string;
  notifications: Notification[];
  isPolling: boolean;
  pollingInterval: number;
}

// Store atoms
export const sensorState = atom<SensorState>({
  status: null,
  voltage: null,
  health: null,
  lastUpdate: 0,
  isLoading: false,
  error: null,
});

export const systemState = atom<SystemState>({
  status: null,
  lastUpdate: 0,
  isLoading: false,
  error: null,
});

export const configState = atom<ConfigState>({
  config: null,
  lastUpdate: 0,
  isLoading: false,
  error: null,
});

export const analyticsState = atom<AnalyticsState>({
  data: null,
  lastUpdate: 0,
  isLoading: false,
  error: null,
});

export const uiState = atom<UIState>({
  theme: 'light',
  sidebarOpen: false,
  activeTab: 'dashboard',
  notifications: [],
  isPolling: false,
  pollingInterval: 1000,
});

// Combined store
export const adaStore = {
  sensor: sensorState,
  system: systemState,
  config: configState,
  analytics: analyticsState,
  ui: uiState,
};

// Action functions
let pollingTimer: number | null = null;

export const startPolling = () => {
  if (pollingTimer) return; // Already polling
  
  const interval = uiState.value.pollingInterval;
  uiState.update(state => ({ ...state, isPolling: true }));
  
  pollingTimer = window.setInterval(() => {
    fetchSensorData();
    fetchSystemStatus();
  }, interval);
};

export const stopPolling = () => {
  if (pollingTimer) {
    clearInterval(pollingTimer);
    pollingTimer = null;
  }
  uiState.update(state => ({ ...state, isPolling: false }));
};

export const setPollingInterval = (interval: number) => {
  uiState.update(state => ({ ...state, pollingInterval: interval }));
  if (uiState.value.isPolling) {
    stopPolling();
    startPolling();
  }
};

// Sensor actions
export const fetchSensorData = async () => {
  sensorState.update(state => ({ ...state, isLoading: true }));
  
  try {
    const [voltageResponse, healthResponse] = await Promise.all([
      adaApi.get('/api/analog-voltage'),
      adaApi.get('/api/sensors/health').catch(() => null) // Health might not be available
    ]);

    sensorState.update(state => ({
      ...state,
      voltage: voltageResponse,
      health: healthResponse,
      lastUpdate: Date.now(),
      isLoading: false,
      error: null,
    }));

    return { voltage: voltageResponse, health: healthResponse };
  } catch (error) {
    sensorState.update(state => ({
      ...state,
      isLoading: false,
      error: error instanceof Error ? error.message : 'Failed to fetch sensor data',
    }));
    console.warn('Sensor data not available:', error);
    return null;
  }
};

// System actions
export const fetchSystemStatus = async () => {
  systemState.update(state => ({ ...state, isLoading: true }));
  
  try {
    const statusResponse = await adaApi.get('/api/status');

    systemState.update(state => ({
      ...state,
      status: statusResponse,
      lastUpdate: Date.now(),
      isLoading: false,
      error: null,
    }));

    return statusResponse;
  } catch (error) {
    systemState.update(state => ({
      ...state,
      isLoading: false,
      error: error instanceof Error ? error.message : 'Failed to fetch system status',
    }));
    console.warn('System status not available:', error);
    return null;
  }
};

// Config actions
export const fetchConfig = async () => {
  configState.update(state => ({ ...state, isLoading: true }));
  
  try {
    const configResponse = await adaApi.get('/api/config');

    configState.update(state => ({
      ...state,
      config: configResponse,
      lastUpdate: Date.now(),
      isLoading: false,
      error: null,
    }));

    return configResponse;
  } catch (error) {
    configState.update(state => ({
      ...state,
      isLoading: false,
      error: error instanceof Error ? error.message : 'Failed to fetch config',
    }));
    console.warn('Config not available:', error);
    return null;
  }
};

// Analytics actions
export const fetchAnalytics = async (params?: any) => {
  analyticsState.update(state => ({ ...state, isLoading: true }));
  
  try {
    const analyticsResponse = await adaApi.get('/api/analytics/summary', { params });

    analyticsState.update(state => ({
      ...state,
      data: analyticsResponse,
      lastUpdate: Date.now(),
      isLoading: false,
      error: null,
    }));

    return analyticsResponse;
  } catch (error) {
    analyticsState.update(state => ({
      ...state,
      isLoading: false,
      error: error instanceof Error ? error.message : 'Failed to fetch analytics',
    }));
    console.warn('Analytics not available:', error);
    return null;
  }
};

// UI actions
export const setActiveTab = (tab: string) => {
  uiState.update(state => ({ ...state, activeTab: tab }));
};

export const toggleSidebar = () => {
  uiState.update(state => ({ ...state, sidebarOpen: !state.sidebarOpen }));
};

export const addNotification = (notification: Omit<Notification, 'id' | 'timestamp' | 'read'>) => {
  const newNotification: Notification = {
    ...notification,
    id: Math.random().toString(36).substr(2, 9),
    timestamp: Date.now(),
    read: false,
  };

  uiState.update(state => ({
    ...state,
    notifications: [newNotification, ...state.notifications].slice(0, 50), // Keep max 50 notifications
  }));

  // Note: Don't call showToast here to avoid infinite loop
};

export const markNotificationRead = (id: string) => {
  uiState.update(state => ({
    ...state,
    notifications: state.notifications.map(n => 
      n.id === id ? { ...n, read: true } : n
    )
  }));
};

export const clearNotifications = () => {
  uiState.update(state => ({ ...state, notifications: [] }));
};

export const setTheme = (theme: 'light' | 'dark') => {
  uiState.update(state => ({ ...state, theme }));
  
  // Apply theme to document
  if (typeof document !== 'undefined') {
    document.documentElement.setAttribute('data-theme', theme);
  }
};

// Convenience functions for API calls with notifications
export const fetchSensorDataWithNotification = async () => {
  try {
    const result = await fetchSensorData();
    if (result) {
      addNotification({
        type: 'info',
        title: 'Sensor Data Updated',
        message: 'Latest sensor readings retrieved successfully',
      });
    }
    return result;
  } catch (error) {
    addNotification({
      type: 'error',
      title: 'Sensor Data Error',
      message: error instanceof Error ? error.message : 'Failed to fetch sensor data',
    });
    return null;
  }
};

export const fetchSystemStatusWithNotification = async () => {
  try {
    const result = await fetchSystemStatus();
    if (result) {
      addNotification({
        type: 'info',
        title: 'System Status Updated',
        message: 'System status retrieved successfully',
      });
    }
    return result;
  } catch (error) {
    addNotification({
      type: 'error',
      title: 'System Status Error',
      message: error instanceof Error ? error.message : 'Failed to fetch system status',
    });
    return null;
  }
};

export const fetchAnalyticsWithNotification = async (params?: any) => {
  try {
    const result = await fetchAnalytics(params);
    if (result) {
      addNotification({
        type: 'success',
        title: 'Analytics Updated',
        message: 'Analytics data retrieved successfully',
      });
    }
    return result;
  } catch (error) {
    addNotification({
      type: 'error',
      title: 'Analytics Error',
      message: error instanceof Error ? error.message : 'Failed to fetch analytics',
    });
    return null;
  }
};

// Initialize store
export const initializeStore = async () => {
  try {
    // Load initial data
    await Promise.all([
      fetchSystemStatus(),
      fetchSensorData(),
    ]);
    
    // Start polling
    startPolling();
    
    console.log('Store initialized successfully');
  } catch (error) {
    console.error('Failed to initialize store:', error);
    addNotification({
      type: 'error',
      title: 'Initialization Error',
      message: 'Failed to initialize application state',
    });
  }
};