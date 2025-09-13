#include "analytics_manager.h"
#include "sd_manager.h"
#include <math.h>
#include <algorithm>

// Global instance
AnalyticsManager analyticsMgr;

// DataWindow implementation
void DataWindow::addValue(float value, unsigned long timestamp) {
    values[writeIndex] = value;
    timestamps[writeIndex] = timestamp;
    
    writeIndex = (writeIndex + 1) % WINDOW_SIZE;
    
    if (count < WINDOW_SIZE) {
        count++;
    } else {
        isFull = true;
    }
}

void DataWindow::clear() {
    writeIndex = 0;
    count = 0;
    isFull = false;
}

float* DataWindow::getValues(int& size) {
    size = count;
    return values;
}

unsigned long* DataWindow::getTimestamps(int& size) {
    size = count;
    return timestamps;
}

// AnalyticsManager implementation
AnalyticsManager::AnalyticsManager() : 
    initialized(false),
    lastDataCompressionTime(0) {
    
    // Initialize configuration with defaults
    config.enabled = true;
    config.analysisInterval = DEFAULT_ANALYSIS_INTERVAL;
    config.trendPeriod = DEFAULT_TREND_PERIOD;
    config.minSamplesForAnalysis = 10;
    config.enablePrediction = true;
    config.enableOutlierDetection = true;
    config.outlierThreshold = 2.5;  // 2.5 sigma
    config.enableDataCompression = true;
    
    // Initialize timing
    for (int i = 0; i < 3; i++) {
        lastAnalysisTime[i] = 0;
    }
}

bool AnalyticsManager::begin() {
    Serial.println("[Analytics] Initializing analytics manager...");
    
    // Clear all data windows
    for (int i = 0; i < 3; i++) {
        sensorWindows[i].clear();
        
        // Initialize statistics
        stats[i] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0};
        
        // Initialize trends
        trends[i] = {0.0, 0.0, 0.0, 0.0, "unknown", 0.0, config.trendPeriod};
    }
    
    initialized = true;
    Serial.println("[Analytics] Analytics manager initialized successfully");
    return true;
}

void AnalyticsManager::setAnalyticsConfig(const AnalyticsConfig& newConfig) {
    config = newConfig;
    
    if (DEBUG_ENABLED) {
        Serial.printf("[Analytics] Configuration updated: interval=%lu, trend_period=%lu\n", 
                     config.analysisInterval, config.trendPeriod);
    }
}

AnalyticsConfig AnalyticsManager::getAnalyticsConfig() {
    return config;
}

void AnalyticsManager::addDataPoint(int sensorIndex, float value, unsigned long timestamp) {
    if (!initialized || sensorIndex < 0 || sensorIndex >= 3) return;
    
    if (timestamp == 0) {
        timestamp = millis();
    }
    
    // Add to data window
    sensorWindows[sensorIndex].addValue(value, timestamp);
    
    if (DEBUG_ENABLED && sensorWindows[sensorIndex].count % 20 == 0) {
        Serial.printf("[Analytics] Sensor %d: %d data points collected\n", 
                     sensorIndex, sensorWindows[sensorIndex].count);
    }
}

void AnalyticsManager::handle() {
    if (!initialized || !config.enabled) return;
    
    unsigned long currentTime = millis();
    
    // Run analytics for each sensor
    for (int i = 0; i < 3; i++) {
        if (currentTime - lastAnalysisTime[i] >= config.analysisInterval) {
            if (hasEnoughData(i)) {
                calculateStatistics(i);
                calculateTrend(i);
                
                if (config.enableOutlierDetection) {
                    detectOutliers(i);
                }
                
                logAnalyticsData(i);
                lastAnalysisTime[i] = currentTime;
            }
        }
    }
    
    // Data compression
    if (config.enableDataCompression && 
        currentTime - lastDataCompressionTime >= DATA_COMPRESSION_INTERVAL) {
        compressHistoricalData();
        lastDataCompressionTime = currentTime;
    }
}

void AnalyticsManager::calculateStatistics(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    int dataSize;
    float* values = sensorWindows[sensorIndex].getValues(dataSize);
    
    if (dataSize < config.minSamplesForAnalysis) return;
    
    // Calculate basic statistics
    stats[sensorIndex].mean = calculateMean(values, dataSize);
    stats[sensorIndex].median = calculateMedian(values, dataSize);
    stats[sensorIndex].stdDev = calculateStdDev(values, dataSize, stats[sensorIndex].mean);
    stats[sensorIndex].variance = stats[sensorIndex].stdDev * stats[sensorIndex].stdDev;
    
    // Find min and max
    stats[sensorIndex].min = values[0];
    stats[sensorIndex].max = values[0];
    
    for (int i = 1; i < dataSize; i++) {
        if (values[i] < stats[sensorIndex].min) stats[sensorIndex].min = values[i];
        if (values[i] > stats[sensorIndex].max) stats[sensorIndex].max = values[i];
    }
    
    stats[sensorIndex].range = stats[sensorIndex].max - stats[sensorIndex].min;
    
    // Calculate quartiles
    float sortedValues[DataWindow::WINDOW_SIZE];
    memcpy(sortedValues, values, dataSize * sizeof(float));
    std::sort(sortedValues, sortedValues + dataSize);
    
    int q1Index = dataSize / 4;
    int q3Index = 3 * dataSize / 4;
    stats[sensorIndex].q1 = sortedValues[q1Index];
    stats[sensorIndex].q3 = sortedValues[q3Index];
    stats[sensorIndex].iqr = stats[sensorIndex].q3 - stats[sensorIndex].q1;
    
    stats[sensorIndex].sampleCount = dataSize;
    stats[sensorIndex].lastUpdated = millis();
    
    if (DEBUG_ENABLED) {
        Serial.printf("[Analytics] Sensor %d stats: mean=%.2f, std=%.2f, range=%.2f\n", 
                     sensorIndex, stats[sensorIndex].mean, stats[sensorIndex].stdDev, stats[sensorIndex].range);
    }
}

void AnalyticsManager::calculateTrend(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    int dataSize;
    float* values = sensorWindows[sensorIndex].getValues(dataSize);
    unsigned long* timestamps = sensorWindows[sensorIndex].getTimestamps(dataSize);
    
    if (dataSize < config.minSamplesForAnalysis) return;
    
    // Calculate linear regression
    float slope, intercept;
    trends[sensorIndex].correlation = calculateLinearRegression(values, timestamps, dataSize, slope, intercept);
    trends[sensorIndex].slope = slope;
    trends[sensorIndex].intercept = intercept;
    
    // Determine trend direction
    if (abs(slope) < 0.001) {
        trends[sensorIndex].trendDirection = "stable";
    } else if (slope > 0) {
        trends[sensorIndex].trendDirection = "increasing";
    } else {
        trends[sensorIndex].trendDirection = "decreasing";
    }
    
    // Calculate change rate per hour
    unsigned long timeSpan = timestamps[dataSize-1] - timestamps[0];
    if (timeSpan > 0) {
        float valueChange = values[dataSize-1] - values[0];
        trends[sensorIndex].changeRate = (valueChange / (timeSpan / 1000.0)) * 3600.0; // per hour
    }
    
    // Predict next value
    if (config.enablePrediction) {
        unsigned long nextTimestamp = millis() + config.analysisInterval;
        trends[sensorIndex].predictedNext = predictNextValue(sensorIndex, nextTimestamp);
    }
    
    trends[sensorIndex].trendPeriod = config.trendPeriod;
    
    if (DEBUG_ENABLED) {
        Serial.printf("[Analytics] Sensor %d trend: %s, slope=%.4f, correlation=%.3f\n", 
                     sensorIndex, trends[sensorIndex].trendDirection.c_str(), 
                     trends[sensorIndex].slope, trends[sensorIndex].correlation);
    }
}

float AnalyticsManager::calculateMean(float* values, int count) {
    if (count == 0) return 0.0;
    
    float sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += values[i];
    }
    return sum / count;
}

float AnalyticsManager::calculateMedian(float* values, int count) {
    if (count == 0) return 0.0;
    
    float sortedValues[DataWindow::WINDOW_SIZE];
    memcpy(sortedValues, values, count * sizeof(float));
    std::sort(sortedValues, sortedValues + count);
    
    if (count % 2 == 0) {
        return (sortedValues[count/2 - 1] + sortedValues[count/2]) / 2.0;
    } else {
        return sortedValues[count/2];
    }
}

float AnalyticsManager::calculateStdDev(float* values, int count, float mean) {
    if (count <= 1) return 0.0;
    
    float sumSquaredDiff = 0.0;
    for (int i = 0; i < count; i++) {
        float diff = values[i] - mean;
        sumSquaredDiff += diff * diff;
    }
    
    return sqrt(sumSquaredDiff / (count - 1));
}

float AnalyticsManager::calculateLinearRegression(float* values, unsigned long* timestamps, int count, float& slope, float& intercept) {
    if (count < 2) {
        slope = 0.0;
        intercept = 0.0;
        return 0.0;
    }
    
    // Convert timestamps to relative time in seconds
    unsigned long baseTime = timestamps[0];
    float* timeValues = new float[count];
    for (int i = 0; i < count; i++) {
        timeValues[i] = (timestamps[i] - baseTime) / 1000.0;
    }
    
    // Calculate means
    float meanX = calculateMean(timeValues, count);
    float meanY = calculateMean(values, count);
    
    // Calculate slope and intercept
    float numerator = 0.0;
    float denominator = 0.0;
    
    for (int i = 0; i < count; i++) {
        float xDiff = timeValues[i] - meanX;
        float yDiff = values[i] - meanY;
        numerator += xDiff * yDiff;
        denominator += xDiff * xDiff;
    }
    
    slope = (denominator != 0) ? numerator / denominator : 0.0;
    intercept = meanY - slope * meanX;
    
    // Calculate correlation coefficient
    float correlation = calculateCorrelation(values, timestamps, count);
    
    delete[] timeValues;
    return correlation;
}

float AnalyticsManager::calculateCorrelation(float* values, unsigned long* timestamps, int count) {
    if (count < 2) return 0.0;
    
    // Convert timestamps to relative time
    unsigned long baseTime = timestamps[0];
    float* timeValues = new float[count];
    for (int i = 0; i < count; i++) {
        timeValues[i] = (timestamps[i] - baseTime) / 1000.0;
    }
    
    float meanX = calculateMean(timeValues, count);
    float meanY = calculateMean(values, count);
    
    float numerator = 0.0;
    float sumXSquared = 0.0;
    float sumYSquared = 0.0;
    
    for (int i = 0; i < count; i++) {
        float xDiff = timeValues[i] - meanX;
        float yDiff = values[i] - meanY;
        numerator += xDiff * yDiff;
        sumXSquared += xDiff * xDiff;
        sumYSquared += yDiff * yDiff;
    }
    
    float denominator = sqrt(sumXSquared * sumYSquared);
    float correlation = (denominator != 0) ? numerator / denominator : 0.0;
    
    delete[] timeValues;
    return correlation;
}

void AnalyticsManager::detectOutliers(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return;
    
    float mean = stats[sensorIndex].mean;
    float stdDev = stats[sensorIndex].stdDev;
    
    if (stdDev == 0.0) return;
    
    int dataSize;
    float* values = sensorWindows[sensorIndex].getValues(dataSize);
    
    int outlierCount = 0;
    for (int i = 0; i < dataSize; i++) {
        float zScore = abs(values[i] - mean) / stdDev;
        if (zScore > config.outlierThreshold) {
            outlierCount++;
        }
    }
    
    if (outlierCount > 0 && DEBUG_ENABLED) {
        Serial.printf("[Analytics] Sensor %d: %d outliers detected (%.1f%% of data)\n", 
                     sensorIndex, outlierCount, (outlierCount * 100.0) / dataSize);
    }
}

void AnalyticsManager::compressHistoricalData() {
    // This could implement data compression strategies
    // For now, we'll just log that compression would happen
    if (DEBUG_ENABLED) {
        Serial.println("[Analytics] Historical data compression cycle (placeholder)");
    }
}

void AnalyticsManager::logAnalyticsData(int sensorIndex) {
    if (!initialized) return;
    
    extern SDManager sdMgr;
    if (!sdMgr.isMounted()) return;
    
    String analyticsLog = "ANALYTICS," + String(sensorIndex) + ",";
    analyticsLog += String(stats[sensorIndex].mean, 3) + ",";
    analyticsLog += String(stats[sensorIndex].stdDev, 3) + ",";
    analyticsLog += String(trends[sensorIndex].slope, 6) + ",";
    analyticsLog += trends[sensorIndex].trendDirection + ",";
    analyticsLog += String(trends[sensorIndex].correlation, 3);
    
    sdMgr.logDataWithTimestamp(analyticsLog);
}

// Public methods for data access
StatisticalData AnalyticsManager::getStatistics(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) {
        return {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0};
    }
    return stats[sensorIndex];
}

TrendData AnalyticsManager::getTrend(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) {
        return {0.0, 0.0, 0.0, 0.0, "unknown", 0.0, 0};
    }
    return trends[sensorIndex];
}

String AnalyticsManager::getStatisticsJSON(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "{}";
    
    DynamicJsonDocument doc(1024);
    StatisticalData& stat = stats[sensorIndex];
    
    doc["sensor"] = sensorIndex;
    doc["mean"] = stat.mean;
    doc["median"] = stat.median;
    doc["std_dev"] = stat.stdDev;
    doc["variance"] = stat.variance;
    doc["min"] = stat.min;
    doc["max"] = stat.max;
    doc["range"] = stat.range;
    doc["q1"] = stat.q1;
    doc["q3"] = stat.q3;
    doc["iqr"] = stat.iqr;
    doc["sample_count"] = stat.sampleCount;
    doc["last_updated"] = stat.lastUpdated;
    
    String json;
    serializeJson(doc, json);
    return json;
}

String AnalyticsManager::getTrendJSON(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return "{}";
    
    DynamicJsonDocument doc(512);
    TrendData& trend = trends[sensorIndex];
    
    doc["sensor"] = sensorIndex;
    doc["slope"] = trend.slope;
    doc["intercept"] = trend.intercept;
    doc["correlation"] = trend.correlation;
    doc["predicted_next"] = trend.predictedNext;
    doc["direction"] = trend.trendDirection;
    doc["change_rate_per_hour"] = trend.changeRate;
    doc["trend_period"] = trend.trendPeriod;
    
    String json;
    serializeJson(doc, json);
    return json;
}

float AnalyticsManager::predictNextValue(int sensorIndex, unsigned long futureTimestamp) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0.0;
    
    TrendData& trend = trends[sensorIndex];
    
    // Simple linear prediction based on trend
    unsigned long currentTime = millis();
    float timeOffset = (futureTimestamp - currentTime) / 1000.0; // seconds
    
    return stats[sensorIndex].mean + (trend.slope * timeOffset);
}

String AnalyticsManager::getAnalyticsSummary() {
    DynamicJsonDocument doc(2048);
    
    doc["analytics_enabled"] = config.enabled;
    doc["analysis_interval"] = config.analysisInterval;
    doc["last_compression"] = lastDataCompressionTime;
    
    JsonArray sensors = doc.createNestedArray("sensors");
    
    for (int i = 0; i < 3; i++) {
        JsonObject sensor = sensors.createNestedObject();
        sensor["id"] = i;
        sensor["data_points"] = sensorWindows[i].count;
        sensor["has_enough_data"] = hasEnoughData(i);
        sensor["last_analysis"] = lastAnalysisTime[i];
        
        if (hasEnoughData(i)) {
            sensor["mean"] = stats[i].mean;
            sensor["std_dev"] = stats[i].stdDev;
            sensor["trend"] = trends[i].trendDirection;
            sensor["correlation"] = trends[i].correlation;
        }
    }
    
    String json;
    serializeJson(doc, json);
    return json;
}

// Utility methods
void AnalyticsManager::clearSensorData(int sensorIndex) {
    if (sensorIndex >= 0 && sensorIndex < 3) {
        sensorWindows[sensorIndex].clear();
        lastAnalysisTime[sensorIndex] = 0;
    }
}

void AnalyticsManager::clearAllData() {
    for (int i = 0; i < 3; i++) {
        clearSensorData(i);
    }
    lastDataCompressionTime = 0;
}

bool AnalyticsManager::hasEnoughData(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return false;
    return sensorWindows[sensorIndex].count >= config.minSamplesForAnalysis;
}

int AnalyticsManager::getDataCount(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0;
    return sensorWindows[sensorIndex].count;
}

bool AnalyticsManager::isInitialized() const {
    return initialized;
}

String AnalyticsManager::getStatus() {
    DynamicJsonDocument doc(512);
    
    doc["initialized"] = initialized;
    doc["enabled"] = config.enabled;
    doc["analysis_interval"] = config.analysisInterval;
    doc["prediction_enabled"] = config.enablePrediction;
    doc["outlier_detection"] = config.enableOutlierDetection;
    
    String status;
    serializeJson(doc, status);
    return status;
}

unsigned long AnalyticsManager::getLastAnalysisTime(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= 3) return 0;
    return lastAnalysisTime[sensorIndex];
}