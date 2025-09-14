// Using global Chart from CDN (Chart.js v3)
// Chart.js will be loaded via CDN in layout
declare const Chart: any;

import { format, subHours, subDays } from 'date-fns';

export interface ChartDataPoint {
  x: number | Date;
  y: number;
}

export interface SensorChartOptions {
  container: HTMLCanvasElement;
  title: string;
  yAxisLabel?: string;
  color?: string;
  timeRange?: '1h' | '6h' | '24h' | '7d';
  realtime?: boolean;
}

export class SensorChart {
  private chart: Chart;
  private options: SensorChartOptions;
  private data: ChartDataPoint[] = [];

  constructor(options: SensorChartOptions) {
    this.options = options;
    this.initChart();
  }

  private initChart() {
    const ctx = this.options.container.getContext('2d')!;
    
    this.chart = new Chart(ctx, {
      type: 'line',
      data: {
        datasets: [{
          label: this.options.title,
          data: this.data,
          borderColor: this.options.color || '#3b82f6',
          backgroundColor: this.options.color ? `${this.options.color}20` : '#3b82f620',
          borderWidth: 2,
          fill: true,
          tension: 0.4,
          pointRadius: 2,
          pointHoverRadius: 6,
        }]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: {
          duration: this.options.realtime ? 0 : 750,
        },
        interaction: {
          intersect: false,
          mode: 'index',
        },
        plugins: {
          title: {
            display: true,
            text: this.options.title,
            font: {
              size: 16,
              weight: 'bold',
            },
          },
          legend: {
            display: false,
          },
          tooltip: {
            callbacks: {
              title: (context) => {
                const date = new Date(context[0].parsed.x);
                return format(date, 'PPp');
              },
              label: (context) => {
                return `${this.options.yAxisLabel || 'Value'}: ${context.parsed.y.toFixed(3)}`;
              },
            },
          },
        },
        scales: {
          x: {
            type: 'time',
            time: {
              displayFormats: {
                minute: 'HH:mm',
                hour: 'HH:mm',
                day: 'MMM dd',
              },
            },
            title: {
              display: true,
              text: 'Time',
            },
            grid: {
              color: '#f3f4f6',
            },
          },
          y: {
            beginAtZero: false,
            title: {
              display: true,
              text: this.options.yAxisLabel || 'Value',
            },
            grid: {
              color: '#f3f4f6',
            },
          },
        },
      },
    });
  }

  updateData(newData: ChartDataPoint[]) {
    this.data = newData.map(point => ({
      x: point.x instanceof Date ? point.x : new Date(point.x),
      y: point.y,
    }));

    this.chart.data.datasets[0].data = this.data;
    this.updateTimeRange();
    this.chart.update();
  }

  addDataPoint(point: ChartDataPoint) {
    const dataPoint = {
      x: point.x instanceof Date ? point.x : new Date(point.x),
      y: point.y,
    };

    this.data.push(dataPoint);

    // Limit data points based on time range
    const maxPoints = this.getMaxPointsForTimeRange();
    if (this.data.length > maxPoints) {
      this.data = this.data.slice(-maxPoints);
    }

    this.chart.data.datasets[0].data = this.data;
    this.updateTimeRange();
    this.chart.update('none');
  }

  private getMaxPointsForTimeRange(): number {
    switch (this.options.timeRange) {
      case '1h': return 60; // 1 point per minute
      case '6h': return 360; // 1 point per minute
      case '24h': return 288; // 1 point per 5 minutes
      case '7d': return 168; // 1 point per hour
      default: return 100;
    }
  }

  private updateTimeRange() {
    if (!this.options.timeRange) return;

    const now = new Date();
    let minTime: Date;

    switch (this.options.timeRange) {
      case '1h':
        minTime = subHours(now, 1);
        break;
      case '6h':
        minTime = subHours(now, 6);
        break;
      case '24h':
        minTime = subDays(now, 1);
        break;
      case '7d':
        minTime = subDays(now, 7);
        break;
      default:
        return;
    }

    this.chart.options.scales!.x!.min = minTime;
    this.chart.options.scales!.x!.max = now;
  }

  setTimeRange(range: '1h' | '6h' | '24h' | '7d') {
    this.options.timeRange = range;
    this.updateTimeRange();
    this.chart.update();
  }

  destroy() {
    this.chart.destroy();
  }

  resize() {
    this.chart.resize();
  }

  // Static helper to create charts for common sensor types
  static createVoltageChart(container: HTMLCanvasElement, options: Partial<SensorChartOptions> = {}) {
    return new SensorChart({
      container,
      title: 'Voltage Readings',
      yAxisLabel: 'Voltage (V)',
      color: '#10b981',
      timeRange: '1h',
      realtime: true,
      ...options,
    });
  }

  static createTemperatureChart(container: HTMLCanvasElement, options: Partial<SensorChartOptions> = {}) {
    return new SensorChart({
      container,
      title: 'Temperature',
      yAxisLabel: 'Temperature (°C)',
      color: '#f59e0b',
      timeRange: '1h',
      realtime: true,
      ...options,
    });
  }

  static createCurrentChart(container: HTMLCanvasElement, options: Partial<SensorChartOptions> = {}) {
    return new SensorChart({
      container,
      title: 'Current Readings',
      yAxisLabel: 'Current (A)',
      color: '#ef4444',
      timeRange: '1h',
      realtime: true,
      ...options,
    });
  }
}

// Global chart management
export class ChartManager {
  private charts: Map<string, SensorChart> = new Map();

  createChart(id: string, options: SensorChartOptions): SensorChart {
    if (this.charts.has(id)) {
      this.charts.get(id)!.destroy();
    }

    const chart = new SensorChart(options);
    this.charts.set(id, chart);
    return chart;
  }

  getChart(id: string): SensorChart | undefined {
    return this.charts.get(id);
  }

  destroyChart(id: string) {
    const chart = this.charts.get(id);
    if (chart) {
      chart.destroy();
      this.charts.delete(id);
    }
  }

  destroyAll() {
    for (const [id, chart] of this.charts) {
      chart.destroy();
    }
    this.charts.clear();
  }

  resizeAll() {
    for (const chart of this.charts.values()) {
      chart.resize();
    }
  }
}

// Global instance
export const chartManager = new ChartManager();

// Auto-resize charts on window resize
window.addEventListener('resize', () => {
  chartManager.resizeAll();
});

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
  chartManager.destroyAll();
});