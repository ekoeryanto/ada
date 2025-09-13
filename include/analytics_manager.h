#ifndef ANALYTICS_MANAGER_H
#define ANALYTICS_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// Analytics data structures
struct StatisticalData {
    float mean;
    float median;
    float stdDev;
    float variance;
    float min;
    float max;
    float range;
    float q1;       // First quartile
    float q3;       // Third quartile
    float iqr;      // Interquartile range
    unsigned long sampleCount;
    unsigned long lastUpdated;
};

struct TrendData {
    float slope;            // Linear regression slope
    float intercept;        // Linear regression intercept
    float correlation;      // Correlation coefficient
    float predictedNext;    // Predicted next value
    String trendDirection;  // "increasing", "decreasing", "stable"
    float changeRate;       // Rate of change per hour
    unsigned long trendPeriod; // Time period for trend calculation
};

struct DataWindow {
    static const int WINDOW_SIZE = 100;  // Configurable window size
    float values[WINDOW_SIZE];
    unsigned long timestamps[WINDOW_SIZE];
    int writeIndex;
    int count;
    bool isFull;
    
    void addValue(float value, unsigned long timestamp);
    void clear();
    float* getValues(int& size);
    unsigned long* getTimestamps(int& size);
};

struct AnalyticsConfig {
    bool enabled;
    unsigned long analysisInterval;     // How often to run analytics (ms)
    unsigned long trendPeriod;          // Period for trend analysis (ms)
    int minSamplesForAnalysis;          // Minimum samples needed
    bool enablePrediction;              // Enable predictive analytics
    bool enableOutlierDetection;        // Enable outlier detection in analytics
    float outlierThreshold;             // Z-score threshold for outliers
    bool enableDataCompression;         // Enable data compression for storage
};

class AnalyticsManager {
private:
    // Data windows for each sensor
    DataWindow sensorWindows[3];
    
    // Statistical data for each sensor
    StatisticalData stats[3];
    TrendData trends[3];
    
    // Analytics configuration
    AnalyticsConfig config;
    
    // Timing
    unsigned long lastAnalysisTime[3];
    unsigned long lastDataCompressionTime;
    bool initialized;
    
    // Static constants
    static const unsigned long DEFAULT_ANALYSIS_INTERVAL = 30000;  // 30 seconds
    static const unsigned long DEFAULT_TREND_PERIOD = 3600000;     // 1 hour
    static const unsigned long DATA_COMPRESSION_INTERVAL = 300000; // 5 minutes
    
    // Private methods
    void calculateStatistics(int sensorIndex);
    void calculateTrend(int sensorIndex);
    float calculateMean(float* values, int count);
    float calculateMedian(float* values, int count);
    float calculateStdDev(float* values, int count, float mean);
    float calculateLinearRegression(float* values, unsigned long* timestamps, int count, float& slope, float& intercept);
    float calculateCorrelation(float* values, unsigned long* timestamps, int count);
    void detectOutliers(int sensorIndex);
    void compressHistoricalData();
    void logAnalyticsData(int sensorIndex);
    
public:
    AnalyticsManager();
    
    // Initialization and configuration
    bool begin();
    void setAnalyticsConfig(const AnalyticsConfig& newConfig);
    AnalyticsConfig getAnalyticsConfig();
    
    // Data input
    void addDataPoint(int sensorIndex, float value, unsigned long timestamp = 0);
    void handle();  // Main analytics processing loop
    
    // Statistics access
    StatisticalData getStatistics(int sensorIndex);
    TrendData getTrend(int sensorIndex);
    String getStatisticsJSON(int sensorIndex);
    String getTrendJSON(int sensorIndex);
    String getAnalyticsReport(int sensorIndex);
    String getAnalyticsSummary();
    
    // Predictive analytics
    float predictNextValue(int sensorIndex, unsigned long futureTimestamp);
    String getPredictionJSON(int sensorIndex, unsigned long futureTimestamp);
    
    // Data management
    void clearSensorData(int sensorIndex);
    void clearAllData();
    void resetAnalytics();
    int getDataCount(int sensorIndex);
    bool hasEnoughData(int sensorIndex);
    
    // Export and reporting
    String exportDataCSV(int sensorIndex, unsigned long startTime = 0, unsigned long endTime = 0);
    String generateReport(unsigned long startTime, unsigned long endTime);
    
    // Status and diagnostics
    bool isInitialized() const;
    String getStatus();
    unsigned long getLastAnalysisTime(int sensorIndex);
};

// Global instance
extern AnalyticsManager analyticsMgr;

#endif // ANALYTICS_MANAGER_H