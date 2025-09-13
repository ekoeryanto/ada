#ifndef WEBHOOK_HANDLER_H
#define WEBHOOK_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "config.h"

// Webhook event types
enum WebhookEventType {
    WEBHOOK_SENSOR_DATA = 0,
    WEBHOOK_ALARM_TRIGGERED = 1,
    WEBHOOK_ALARM_CLEARED = 2,
    WEBHOOK_HEALTH_ALERT = 3,
    WEBHOOK_CALIBRATION_EVENT = 4,
    WEBHOOK_SYSTEM_EVENT = 5,
    WEBHOOK_DIAGNOSTIC_ALERT = 6,
    WEBHOOK_ANALYTICS_REPORT = 7
};

// Webhook delivery status
enum WebhookStatus {
    WEBHOOK_PENDING = 0,
    WEBHOOK_SUCCESS = 1,
    WEBHOOK_FAILED = 2,
    WEBHOOK_RETRY = 3,
    WEBHOOK_MAX_RETRIES = 4
};

// Webhook configuration
struct WebhookConfig {
    String url;                     // Webhook endpoint URL
    String secret;                  // Optional webhook secret for authentication
    String authToken;               // Optional bearer token
    String customHeaders;           // Optional custom headers (JSON format)
    bool enabled;                   // Enable/disable webhook
    unsigned long timeout;          // HTTP timeout in milliseconds
    int maxRetries;                 // Maximum retry attempts
    unsigned long retryDelay;       // Delay between retries in milliseconds
    bool validateSSL;               // SSL certificate validation
    WebhookEventType eventTypes[8]; // Array of enabled event types
    int eventTypeCount;             // Number of enabled event types
};

// Webhook delivery queue item
struct WebhookDelivery {
    String payload;                 // JSON payload to send
    WebhookEventType eventType;     // Type of event
    unsigned long timestamp;        // When the event occurred
    WebhookStatus status;           // Delivery status
    int retryCount;                 // Current retry attempt
    unsigned long nextRetry;        // Next retry time
    String webhookId;               // Unique webhook ID
    String eventId;                 // Unique event ID
    String response;                // Server response (for debugging)
    int httpCode;                   // HTTP response code
};

class WebhookHandler {
private:
    // Configuration
    static const int MAX_WEBHOOKS = 5;
    static const int MAX_QUEUE_SIZE = 50;
    WebhookConfig webhooks[MAX_WEBHOOKS];
    int webhookCount;
    
    // Delivery queue
    WebhookDelivery deliveryQueue[MAX_QUEUE_SIZE];
    int queueSize;
    int queueIndex;
    
    // Statistics
    unsigned long totalSent;
    unsigned long totalSuccess;
    unsigned long totalFailed;
    unsigned long totalRetries;
    unsigned long lastDeliveryTime;
    
    // State
    bool initialized;
    unsigned long lastProcessTime;
    HTTPClient httpClient;
    
    // Helper methods
    String generateEventId();
    String generateWebhookId();
    bool isEventTypeEnabled(const String& webhookId, WebhookEventType eventType);
    String createSignature(const String& payload, const String& secret);
    bool sendWebhook(WebhookDelivery& delivery, const WebhookConfig& config);
    void addToQueue(const String& payload, WebhookEventType eventType, const String& webhookId = "");
    void processQueue();
    void cleanupQueue();
    void updateDeliveryStatus(WebhookDelivery& delivery, WebhookStatus status, const String& response = "", int httpCode = 0);
    
public:
    WebhookHandler();
    
    // Initialization
    bool begin();
    void reset();
    
    // Webhook configuration
    String addWebhook(const String& url, const String& secret = "", const String& authToken = "");
    bool removeWebhook(const String& webhookId);
    bool updateWebhook(const String& webhookId, const WebhookConfig& config);
    WebhookConfig getWebhook(const String& webhookId);
    String getWebhooksJSON();
    bool configureWebhook(const String& webhookId, const String& configJSON);
    
    // Event type management
    bool enableEventType(const String& webhookId, WebhookEventType eventType);
    bool disableEventType(const String& webhookId, WebhookEventType eventType);
    bool setEventTypes(const String& webhookId, WebhookEventType* eventTypes, int count);
    
    // Event sending
    bool sendSensorData(const String& sensorId, float value, const String& unit, unsigned long timestamp);
    bool sendAlarmTriggered(const String& sensorId, const String& alarmType, float value, float threshold);
    bool sendAlarmCleared(const String& sensorId, const String& alarmType, float value);
    bool sendHealthAlert(const String& component, const String& message, const String& severity);
    bool sendCalibrationEvent(const String& sensorId, const String& eventType, const String& details);
    bool sendSystemEvent(const String& eventType, const String& message, const String& details = "");
    bool sendDiagnosticAlert(const String& category, const String& message, const String& level);
    bool sendAnalyticsReport(const String& reportType, const String& reportData);
    bool sendCustomEvent(WebhookEventType eventType, const String& payload);
    
    // Queue management
    void handle();
    void clearQueue();
    void retryFailed();
    String getQueueStatus();
    int getQueueSize();
    
    // Statistics
    String getStatistics();
    void resetStatistics();
    unsigned long getTotalSent();
    unsigned long getTotalSuccess();
    unsigned long getTotalFailed();
    float getSuccessRate();
    
    // Configuration
    void setGlobalTimeout(unsigned long timeout);
    void setGlobalRetryPolicy(int maxRetries, unsigned long retryDelay);
    void enableSSLValidation(bool enable);
    
    // Testing
    bool testWebhook(const String& webhookId);
    String getLastResponses(int count = 5);
    
    // Status
    bool isInitialized() const;
    String getStatus();
    
    // JSON exports
    String getDeliveryQueueJSON();
    String getWebhookConfigJSON();
    String getEventTypesJSON();
};

// Event type helper functions
String webhookEventTypeToString(WebhookEventType eventType);
WebhookEventType stringToWebhookEventType(const String& eventTypeStr);
String webhookStatusToString(WebhookStatus status);

// Global instance
extern WebhookHandler webhookHandler;

#endif // WEBHOOK_HANDLER_H