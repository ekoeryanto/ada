declare global {
  interface Window {
    ADA?: any;
    adaStore?: any;
    adaActions?: {
      restartSystem: () => Promise<void>;
      resetWifi: () => Promise<void>;
      calibrateSensor: (sensorId: number, calibrationData: any) => Promise<void>;
      fetchSensorData: () => Promise<void>;
      fetchSystemStatus: () => Promise<void>;
      fetchConfig: () => Promise<void>;
      fetchAnalytics: (params?: any) => Promise<void>;
      addNotification: (notification: any) => void;
      setTheme: (theme: 'light' | 'dark') => void;
      toggleSidebar: () => void;
    };
    showToast?: (message: string, type?: string) => void;
    hideToast?: (id: string) => void;
    clearToasts?: () => void;
    notificationManager?: any;
    restartSystem?: () => Promise<void>;
    resetWifi?: () => Promise<void>;
    calibrateSensor?: (sensorId: string) => Promise<void>;
    Chart?: any; // Chart.js from CDN
  }
}

export {};