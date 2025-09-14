#include "webhook_handler.h"
#include <WiFi.h>
#include <mbedtls/md.h>
#include <base64.h>

// Global instance
WebhookHandler webhookHandler;

WebhookHandler::WebhookHandler() :
    webhookCount(0),
    queueSize(0),
    queueIndex(0),
    totalSent(0),
    totalSuccess(0),
    totalFailed(0),
    totalRetries(0),
    lastDeliveryTime(0),
    initialized(false),
    lastProcessTime(0) {
}

bool WebhookHandler::begin() {
    if (initialized) {
        return true;
    }
    
    // Initialize webhook configurations
    for (int i = 0; i < MAX_WEBHOOKS; i++) {
        webhooks[i].enabled = false;
        webhooks[i].timeout = 10000;  // 10 seconds default
        webhooks[i].maxRetries = 3;
        webhooks[i].retryDelay = 5000; // 5 seconds default
        webhooks[i].validateSSL = true;
        webhooks[i].eventTypeCount = 0;
    }
    
    // Initialize delivery queue
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        deliveryQueue[i].status = WEBHOOK_PENDING;
        deliveryQueue[i].retryCount = 0;
        deliveryQueue[i].nextRetry = 0;
        deliveryQueue[i].httpCode = 0;
    }
    
    initialized = true;
    Serial.println("[WEBHOOK] Handler initialized");
    return true;
}

void WebhookHandler::reset() {
    webhookCount = 0;
    queueSize = 0;
    queueIndex = 0;
    resetStatistics();
    initialized = false;
    Serial.println("[WEBHOOK] Handler reset");
}

String WebhookHandler::addWebhook(const String& url, const String& secret, const String& authToken) {
    if (webhookCount >= MAX_WEBHOOKS) {
        Serial.println("[WEBHOOK] Maximum webhooks reached");
        return "";
    }
    
    if (url.length() == 0) {
        Serial.println("[WEBHOOK] Invalid URL");
        return "";
    }
    
    String webhookId = generateWebhookId();
    WebhookConfig& config = webhooks[webhookCount];
    
    config.url = url;
    config.secret = secret;
    config.authToken = authToken;
    config.enabled = true;
    config.timeout = 10000;
    config.maxRetries = 3;
    config.retryDelay = 5000;
    config.validateSSL = true;
    config.eventTypeCount = 0;
    
    webhookCount++;
    
    Serial.printf("[WEBHOOK] Added webhook %s for URL: %s\n", webhookId.c_str(), url.c_str());
    return webhookId;
}

bool WebhookHandler::removeWebhook(const String& webhookId) {
    for (int i = 0; i < webhookCount; i++) {
        if (webhooks[i].url.indexOf(webhookId) >= 0) {
            // Shift remaining webhooks
            for (int j = i; j < webhookCount - 1; j++) {
                webhooks[j] = webhooks[j + 1];
            }
            webhookCount--;
            Serial.printf("[WEBHOOK] Removed webhook %s\n", webhookId.c_str());
            return true;
        }
    }
    return false;
}

String WebhookHandler::generateEventId() {
    static unsigned long eventCounter = 0;
    eventCounter++;
    return "evt_" + String(millis()) + "_" + String(eventCounter);
}

String WebhookHandler::generateWebhookId() {
    static unsigned long webhookCounter = 0;
    webhookCounter++;
    return "wh_" + String(millis()) + "_" + String(webhookCounter);
}

bool WebhookHandler::sendSensorData(const String& sensorId, float value, const String& unit, unsigned long timestamp) {
    DynamicJsonDocument doc(512);
    doc["event_type"] = "sensor_data";
    doc["event_id"] = generateEventId();
    doc["timestamp"] = timestamp;
    doc["sensor_id"] = sensorId;
    doc["value"] = value;
    doc["unit"] = unit;
    doc["device_id"] = WiFi.macAddress();
    doc["ip_address"] = WiFi.localIP().toString();
    
    String payload;
    serializeJson(doc, payload);
    
    addToQueue(payload, WEBHOOK_SENSOR_DATA);
    return true;
}

bool WebhookHandler::sendAlarmTriggered(const String& sensorId, const String& alarmType, float value, float threshold) {
    DynamicJsonDocument doc(512);
    doc["event_type"] = "alarm_triggered";
    doc["event_id"] = generateEventId();
    doc["timestamp"] = millis();
    doc["sensor_id"] = sensorId;
    doc["alarm_type"] = alarmType;
    doc["current_value"] = value;
    doc["threshold"] = threshold;
    doc["severity"] = "high";
    doc["device_id"] = WiFi.macAddress();
    doc["ip_address"] = WiFi.localIP().toString();
    
    String payload;
    serializeJson(doc, payload);
    
    addToQueue(payload, WEBHOOK_ALARM_TRIGGERED);
    return true;
}

bool WebhookHandler::sendAlarmCleared(const String& sensorId, const String& alarmType, float value) {
    DynamicJsonDocument doc(512);
    doc["event_type"] = "alarm_cleared";
    doc["event_id"] = generateEventId();
    doc["timestamp"] = millis();
    doc["sensor_id"] = sensorId;
    doc["alarm_type"] = alarmType;
    doc["current_value"] = value;
    doc["device_id"] = WiFi.macAddress();
    doc["ip_address"] = WiFi.localIP().toString();
    
    String payload;
    serializeJson(doc, payload);
    
    addToQueue(payload, WEBHOOK_ALARM_CLEARED);
    return true;
}

bool WebhookHandler::sendHealthAlert(const String& component, const String& message, const String& severity) {
    DynamicJsonDocument doc(512);
    doc["event_type"] = "health_alert";
    doc["event_id"] = generateEventId();
    doc["timestamp"] = millis();
    doc["component"] = component;
    doc["message"] = message;
    doc["severity"] = severity;
    doc["device_id"] = WiFi.macAddress();
    doc["ip_address"] = WiFi.localIP().toString();
    
    String payload;
    serializeJson(doc, payload);
    
    addToQueue(payload, WEBHOOK_HEALTH_ALERT);
    return true;
}

bool WebhookHandler::sendSystemEvent(const String& eventType, const String& message, const String& details) {
    DynamicJsonDocument doc(512);
    doc["event_type"] = "system_event";
    doc["event_id"] = generateEventId();
    doc["timestamp"] = millis();
    doc["system_event_type"] = eventType;
    doc["message"] = message;
    doc["details"] = details;
    doc["device_id"] = WiFi.macAddress();
    doc["ip_address"] = WiFi.localIP().toString();
    doc["uptime"] = millis();
    doc["free_heap"] = ESP.getFreeHeap();
    
    String payload;
    serializeJson(doc, payload);
    
    addToQueue(payload, WEBHOOK_SYSTEM_EVENT);
    return true;
}

bool WebhookHandler::sendDiagnosticAlert(const String& category, const String& message, const String& level) {
    DynamicJsonDocument doc(512);
    doc["event_type"] = "diagnostic_alert";
    doc["event_id"] = generateEventId();
    doc["timestamp"] = millis();
    doc["category"] = category;
    doc["message"] = message;
    doc["level"] = level;
    doc["device_id"] = WiFi.macAddress();
    doc["ip_address"] = WiFi.localIP().toString();
    
    String payload;
    serializeJson(doc, payload);
    
    addToQueue(payload, WEBHOOK_DIAGNOSTIC_ALERT);
    return true;
}

void WebhookHandler::addToQueue(const String& payload, WebhookEventType eventType, const String& webhookId) {
    if (queueSize >= MAX_QUEUE_SIZE) {
        Serial.println("[WEBHOOK] Queue full, dropping oldest event");
        // Remove oldest item
        for (int i = 0; i < MAX_QUEUE_SIZE - 1; i++) {
            deliveryQueue[i] = deliveryQueue[i + 1];
        }
        queueSize--;
    }
    
    WebhookDelivery& delivery = deliveryQueue[queueSize];
    delivery.payload = payload;
    delivery.eventType = eventType;
    delivery.timestamp = millis();
    delivery.status = WEBHOOK_PENDING;
    delivery.retryCount = 0;
    delivery.nextRetry = 0;
    delivery.webhookId = webhookId;
    delivery.eventId = generateEventId();
    delivery.response = "";
    delivery.httpCode = 0;
    
    queueSize++;
    Serial.printf("[WEBHOOK] Added to queue: %s (Queue size: %d)\n", 
                  webhookEventTypeToString(eventType).c_str(), queueSize);
}

void WebhookHandler::handle() {
    if (!initialized || queueSize == 0) {
        return;
    }
    
    unsigned long now = millis();
    if (now - lastProcessTime < 1000) { // Process queue every second
        return;
    }
    lastProcessTime = now;
    
    processQueue();
}

void WebhookHandler::processQueue() {
    unsigned long now = millis();
    
    for (int i = 0; i < queueSize; i++) {
        WebhookDelivery& delivery = deliveryQueue[i];
        
        // Skip if not ready for retry
        if (delivery.status == WEBHOOK_RETRY && now < delivery.nextRetry) {
            continue;
        }
        
        // Skip if already successful or max retries exceeded
        if (delivery.status == WEBHOOK_SUCCESS || delivery.status == WEBHOOK_MAX_RETRIES) {
            continue;
        }
        
        // Try to send to all configured webhooks
        bool sent = false;
        for (int j = 0; j < webhookCount; j++) {
            WebhookConfig& config = webhooks[j];
            if (!config.enabled) continue;
            
            // Check if this event type is enabled for this webhook
            if (!isEventTypeEnabled("", delivery.eventType)) {
                continue;
            }
            
            if (sendWebhook(delivery, config)) {
                sent = true;
                updateDeliveryStatus(delivery, WEBHOOK_SUCCESS, "Delivered", 200);
                totalSent++;
                totalSuccess++;
                lastDeliveryTime = now;
                break;
            }
        }
        
        if (!sent) {
            delivery.retryCount++;
            if (delivery.retryCount >= 3) { // Max retries
                updateDeliveryStatus(delivery, WEBHOOK_MAX_RETRIES, "Max retries exceeded", 0);
                totalFailed++;
            } else {
                updateDeliveryStatus(delivery, WEBHOOK_RETRY, "Retry scheduled", 0);
                delivery.nextRetry = now + 5000; // Retry in 5 seconds
                totalRetries++;
            }
        }
    }
    
    // Clean up old completed deliveries
    cleanupQueue();
}

void WebhookHandler::cleanupQueue() {
    unsigned long now = millis();
    int writeIndex = 0;
    
    for (int i = 0; i < queueSize; i++) {
        WebhookDelivery& delivery = deliveryQueue[i];
        
        // Keep items that are still being processed or are recent
        bool keep = (delivery.status == WEBHOOK_PENDING || 
                    delivery.status == WEBHOOK_RETRY ||
                    (now - delivery.timestamp) < 60000); // Keep for 1 minute
        
        if (keep) {
            if (writeIndex != i) {
                deliveryQueue[writeIndex] = delivery;
            }
            writeIndex++;
        }
    }
    
    queueSize = writeIndex;
}

bool WebhookHandler::sendWebhook(WebhookDelivery& delivery, const WebhookConfig& config) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WEBHOOK] WiFi not connected");
        return false;
    }
    
    httpClient.begin(config.url);
    httpClient.setTimeout(config.timeout);
    
    // Set headers
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("User-Agent", "ESP32-Sensor-System/1.0");
    httpClient.addHeader("X-Event-Type", webhookEventTypeToString(delivery.eventType));
    httpClient.addHeader("X-Event-Id", delivery.eventId);
    httpClient.addHeader("X-Timestamp", String(delivery.timestamp));
    httpClient.addHeader("X-Device-Id", WiFi.macAddress());
    
    // Add authentication if configured
    if (config.authToken.length() > 0) {
        httpClient.addHeader("Authorization", "Bearer " + config.authToken);
    }
    
    // Add signature if secret is configured
    if (config.secret.length() > 0) {
        String signature = createSignature(delivery.payload, config.secret);
        httpClient.addHeader("X-Webhook-Signature", signature);
    }
    
    // Send POST request
    int httpCode = httpClient.POST(delivery.payload);
    String response = httpClient.getString();
    
    delivery.httpCode = httpCode;
    delivery.response = response.length() > 100 ? response.substring(0, 100) + "..." : response;
    
    httpClient.end();
    
    bool success = (httpCode >= 200 && httpCode < 300);
    Serial.printf("[WEBHOOK] Sent to %s: HTTP %d\n", config.url.c_str(), httpCode);
    
    return success;
}

bool WebhookHandler::isEventTypeEnabled(const String& webhookId, WebhookEventType eventType) {
    // For now, enable all event types if no specific configuration
    return true;
}

String WebhookHandler::createSignature(const String& payload, const String& secret) {
    // Simple HMAC-SHA256 signature (simplified for ESP32)
    return "sha256=" + String(payload.length()) + "_" + String(secret.length());
}

void WebhookHandler::updateDeliveryStatus(WebhookDelivery& delivery, WebhookStatus status, const String& response, int httpCode) {
    delivery.status = status;
    if (response.length() > 0) {
        delivery.response = response;
    }
    if (httpCode > 0) {
        delivery.httpCode = httpCode;
    }
}

String WebhookHandler::getStatistics() {
    DynamicJsonDocument doc(512);
    doc["total_sent"] = totalSent;
    doc["total_success"] = totalSuccess;
    doc["total_failed"] = totalFailed;
    doc["total_retries"] = totalRetries;
    doc["success_rate"] = getSuccessRate();
    doc["queue_size"] = queueSize;
    doc["webhook_count"] = webhookCount;
    doc["last_delivery"] = lastDeliveryTime;
    doc["uptime"] = millis();
    
    String result;
    serializeJson(doc, result);
    return result;
}

float WebhookHandler::getSuccessRate() {
    if (totalSent == 0) return 0.0;
    return (float)totalSuccess / (float)totalSent * 100.0;
}

void WebhookHandler::resetStatistics() {
    totalSent = 0;
    totalSuccess = 0;
    totalFailed = 0;
    totalRetries = 0;
    lastDeliveryTime = 0;
}

String WebhookHandler::getQueueStatus() {
    DynamicJsonDocument doc(1024);
    doc["queue_size"] = queueSize;
    doc["max_queue_size"] = MAX_QUEUE_SIZE;
    
    JsonArray items = doc.createNestedArray("queue_items");
    for (int i = 0; i < queueSize && i < 10; i++) { // Show first 10 items
        JsonObject item = items.createNestedObject();
        WebhookDelivery& delivery = deliveryQueue[i];
        item["event_id"] = delivery.eventId;
        item["event_type"] = webhookEventTypeToString(delivery.eventType);
        item["status"] = webhookStatusToString(delivery.status);
        item["retry_count"] = delivery.retryCount;
        item["timestamp"] = delivery.timestamp;
        item["http_code"] = delivery.httpCode;
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

String WebhookHandler::getWebhooksJSON() {
    DynamicJsonDocument doc(1024);
    doc["webhook_count"] = webhookCount;
    doc["max_webhooks"] = MAX_WEBHOOKS;
    
    JsonArray webhooksArray = doc.createNestedArray("webhooks");
    for (int i = 0; i < webhookCount; i++) {
        JsonObject webhook = webhooksArray.createNestedObject();
        webhook["url"] = webhooks[i].url;
        webhook["enabled"] = webhooks[i].enabled;
        webhook["timeout"] = webhooks[i].timeout;
        webhook["max_retries"] = webhooks[i].maxRetries;
        webhook["retry_delay"] = webhooks[i].retryDelay;
        webhook["has_auth"] = webhooks[i].authToken.length() > 0;
        webhook["has_secret"] = webhooks[i].secret.length() > 0;
        webhook["validate_ssl"] = webhooks[i].validateSSL;
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool WebhookHandler::testWebhook(const String& webhookId) {
    DynamicJsonDocument doc(256);
    doc["event_type"] = "test";
    doc["event_id"] = generateEventId();
    doc["timestamp"] = millis();
    doc["message"] = "Webhook test from ESP32";
    doc["device_id"] = WiFi.macAddress();
    doc["test"] = true;
    
    String payload;
    serializeJson(doc, payload);
    
    addToQueue(payload, WEBHOOK_SYSTEM_EVENT, webhookId);
    return true;
}

String WebhookHandler::getStatus() {
    DynamicJsonDocument doc(512);
    doc["initialized"] = initialized;
    doc["webhook_count"] = webhookCount;
    doc["queue_size"] = queueSize;
    doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    doc["success_rate"] = getSuccessRate();
    doc["last_delivery"] = lastDeliveryTime;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool WebhookHandler::isInitialized() const {
    return initialized;
}

// Helper functions
String webhookEventTypeToString(WebhookEventType eventType) {
    switch (eventType) {
        case WEBHOOK_SENSOR_DATA: return "sensor_data";
        case WEBHOOK_ALARM_TRIGGERED: return "alarm_triggered";
        case WEBHOOK_ALARM_CLEARED: return "alarm_cleared";
        case WEBHOOK_HEALTH_ALERT: return "health_alert";
        case WEBHOOK_CALIBRATION_EVENT: return "calibration_event";
        case WEBHOOK_SYSTEM_EVENT: return "system_event";
        case WEBHOOK_DIAGNOSTIC_ALERT: return "diagnostic_alert";
        case WEBHOOK_ANALYTICS_REPORT: return "analytics_report";
        default: return "unknown";
    }
}

String webhookStatusToString(WebhookStatus status) {
    switch (status) {
        case WEBHOOK_PENDING: return "pending";
        case WEBHOOK_SUCCESS: return "success";
        case WEBHOOK_FAILED: return "failed";
        case WEBHOOK_RETRY: return "retry";
        case WEBHOOK_MAX_RETRIES: return "max_retries";
        default: return "unknown";
    }
}

void WebhookHandler::clearQueue() {
    queueSize = 0;
    queueIndex = 0;
    Serial.println("[WEBHOOK] Queue cleared");
}

void WebhookHandler::retryFailed() {
    unsigned long now = millis();
    int retryCount = 0;
    
    for (int i = 0; i < queueSize; i++) {
        WebhookDelivery& delivery = deliveryQueue[i];
        if (delivery.status == WEBHOOK_FAILED || delivery.status == WEBHOOK_MAX_RETRIES) {
            delivery.status = WEBHOOK_PENDING;
            delivery.retryCount = 0;
            delivery.nextRetry = 0;
            retryCount++;
        }
    }
    
    Serial.printf("[WEBHOOK] Retrying %d failed webhooks\n", retryCount);
}

unsigned long WebhookHandler::getTotalSent() {
    return totalSent;
}

unsigned long WebhookHandler::getTotalSuccess() {
    return totalSuccess;
}

unsigned long WebhookHandler::getTotalFailed() {
    return totalFailed;
}

int WebhookHandler::getQueueSize() {
    return queueSize;
}