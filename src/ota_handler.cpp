#include "ota_handler.h"
#include "system_manager.h"

// Global instance
OTAHandler otaHandler;

OTAHandler::OTAHandler() {
    server = nullptr;
    otaEnabled = false;
    updateInProgress = false;
}

bool OTAHandler::initialize(AsyncWebServer* webServer) {
    if (!webServer) {
        Serial.println("[OTA] Error: WebServer pointer is null");
        return false;
    }
    
    server = webServer;
    
    Serial.println("[OTA] Initializing OTA handler...");
    
    // Setup simple web-based OTA upload page (no ElegantOTA)
    server->on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 OTA Update</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; }
        .upload-form { margin: 20px 0; }
        input[type="file"] { width: 100%; padding: 10px; margin: 10px 0; border: 2px dashed #ddd; border-radius: 4px; }
        input[type="submit"] { background: #007bff; color: white; padding: 12px 24px; border: none; border-radius: 4px; cursor: pointer; width: 100%; font-size: 16px; }
        input[type="submit"]:hover { background: #0056b3; }
        .progress { width: 100%; height: 20px; background: #f0f0f0; border-radius: 10px; margin: 10px 0; display: none; }
        .progress-bar { height: 100%; background: #007bff; border-radius: 10px; width: 0%; transition: width 0.3s; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Firmware Update</h1>
        <form method='POST' action='/update' enctype='multipart/form-data' class="upload-form">
            <input type='file' name='update' accept='.bin' required>
            <input type='submit' value='Update Firmware'>
        </form>
        <div class="progress" id="progress">
            <div class="progress-bar" id="progress-bar"></div>
        </div>
        <div id="status"></div>
    </div>
    <script>
        document.querySelector('form').onsubmit = function() {
            document.getElementById('progress').style.display = 'block';
            document.getElementById('status').innerHTML = 'Uploading...';
        }
    </script>
</body>
</html>
        )";
        request->send(200, "text/html", html);
    });
    
    server->on("/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleOTAResult(request);
    }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        handleOTAUpload(request, filename, index, data, len, final);
    });
    
    // Initialize ArduinoOTA (for command line uploads)
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(3232); // Standard ESP32 OTA port
    
    // Set timeout values for better reliability
    ArduinoOTA.setTimeout(300000);  // 5 minutes timeout (increased from 2)
    
    // Enable mDNS for better discovery
    ArduinoOTA.setMdnsEnabled(true);
    
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("[ArduinoOTA] Start updating " + type);
        Serial.println("[ArduinoOTA] Ready to receive firmware");
        Serial.println("[ArduinoOTA] Starting upload process...");
        Serial.println("[ArduinoOTA] Client connected, beginning transfer...");
        
        // Disable other services during OTA
        otaHandler.updateInProgress = true;
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[ArduinoOTA] Update completed successfully");
        Serial.println("[ArduinoOTA] Restarting...");
        otaHandler.updateInProgress = false;
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static unsigned int lastPercent = 0;
        static unsigned long lastTime = 0;
        unsigned int percent = (progress / (total / 100));
        unsigned long currentTime = millis();
        
        if (percent != lastPercent || (currentTime - lastTime) > 5000) {  // Report every % or every 5 seconds
            Serial.printf("[ArduinoOTA] Progress: %u%% (%u/%u bytes) - %.2f KB/s\n", 
                percent, progress, total, 
                (float)(progress - 0) / ((currentTime - 0) / 1000.0) / 1024.0);
            lastPercent = percent;
            lastTime = currentTime;
        }
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[ArduinoOTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed - Check OTA password");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed - Check partition table");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed - Network connectivity issue");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed - Transfer interrupted");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed - Failed to finalize update");
        }
        
        // Reset state on error
        otaHandler.updateInProgress = false;
        Serial.println("[ArduinoOTA] OTA error occurred, resetting state");
    });
    
    ArduinoOTA.begin();
    
    Serial.println("[ArduinoOTA] Service started successfully");
    Serial.printf("[ArduinoOTA] Listening on port %d\n", 3232);
    Serial.printf("[ArduinoOTA] Hostname: %s\n", HOSTNAME);
    
    otaEnabled = true;
    Serial.println("[OTA] OTA handler initialized successfully");
    Serial.printf("[OTA] Web OTA URL: http://%s/update\n", WiFi.localIP().toString().c_str());
    Serial.printf("[OTA] Command line OTA: IP %s, Port 3232, Password: %s\n", WiFi.localIP().toString().c_str(), OTA_PASSWORD);
    
    return true;
}

void OTAHandler::handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        Serial.printf("[OTA] Web update start: %s\n", filename.c_str());
        updateInProgress = true;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    }
    
    if (Update.write(data, len) != len) {
        Update.printError(Serial);
    }
    
    if (final) {
        if (Update.end(true)) {
            Serial.printf("[OTA] Web update success: %u bytes\n", index + len);
        } else {
            Update.printError(Serial);
        }
        updateInProgress = false;
    }
}

void OTAHandler::handleOTAResult(AsyncWebServerRequest *request) {
    if (Update.hasError()) {
        request->send(500, "text/plain", "Update failed");
    } else {
        request->send(200, "text/plain", "Update successful, restarting...");
        delay(1000);
        ESP.restart();
    }
}

void OTAHandler::handle() {
    // Handle ArduinoOTA for command line uploads
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 60000) {  // Every 60 seconds
        Serial.printf("[OTA] Handler active - ArduinoOTA listening on port 3232\n");
        lastDebug = millis();
    }
    
    ArduinoOTA.handle();
}

void OTAHandler::begin() {
    if (server) {
        otaEnabled = true;
        Serial.println("[OTA] OTA service started");
    }
}

void OTAHandler::end() {
    otaEnabled = false;
    Serial.println("[OTA] OTA service stopped");
}

bool OTAHandler::isEnabled() {
    return otaEnabled;
}

bool OTAHandler::isUpdateInProgress() {
    return updateInProgress;
}

void OTAHandler::enable() {
    otaEnabled = true;
    Serial.println("[OTA] OTA enabled");
}

void OTAHandler::disable() {
    otaEnabled = false;
    Serial.println("[OTA] OTA disabled");
}

String OTAHandler::getUpdateURL() {
    if (WiFi.status() == WL_CONNECTED) {
        return "http://" + WiFi.localIP().toString() + "/update";
    }
    return "Not connected to WiFi";
}

String OTAHandler::getStatus() {
    if (!otaEnabled) {
        return "Disabled";
    } else if (updateInProgress) {
        return "Update in progress";
    } else {
        return "Ready";
    }
}
