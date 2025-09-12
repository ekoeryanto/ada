#include "web_server.h"
#include "ota_handler.h"
#include "system_manager.h"

// Global instance
WebServerHandler webServer;

WebServerHandler::WebServerHandler() 
    : server(WEB_SERVER_PORT)
{
    serverStarted = false;
}

bool WebServerHandler::initialize() {
    Serial.println("[WebServer] Initializing web server...");
    
    setupRoutes();
    
    Serial.printf("[WebServer] Web server initialized on port %d\n", WEB_SERVER_PORT);
    return true;
}

void WebServerHandler::setupRoutes() {
    // AsyncWebServer routes
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String html = generateWebPage();
        request->send(200, "text/html", html);
    });
    
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        doc["project"] = PROJECT_NAME;
        doc["version"] = PROJECT_VERSION;
        doc["author"] = PROJECT_AUTHOR;
        doc["status"] = systemMgr.getStatusString();
        doc["uptime"] = systemMgr.getUptimeString();
        doc["freeHeap"] = ESP.getFreeHeap();
        doc["chipId"] = systemMgr.getChipId();
        
        if (wifiMgr.isConnected()) {
            doc["wifi"]["connected"] = true;
            doc["wifi"]["ssid"] = wifiMgr.getSSID();
            doc["wifi"]["ip"] = wifiMgr.getIP();
            doc["wifi"]["rssi"] = wifiMgr.getRSSI();
        } else {
            doc["wifi"]["connected"] = false;
        }
        
        doc["ota"]["enabled"] = otaHandler.isEnabled();
        doc["ota"]["status"] = otaHandler.getStatus();
        doc["ota"]["url"] = otaHandler.getUpdateURL();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(512);
        doc["hostname"] = HOSTNAME;
        doc["ap_password"] = AP_PASSWORD;
        doc["ota_username"] = OTA_USERNAME;
        doc["web_port"] = WEB_SERVER_PORT;
        doc["debug_enabled"] = DEBUG_ENABLED;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"message\":\"Restarting...\"}");
        delay(1000);
        systemMgr.restart();
    });
    
    server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"message\":\"Resetting WiFi settings...\"}");
        delay(1000);
        wifiMgr.resetWiFiSettings();
        systemMgr.restart();
    });
    
    server.onNotFound([this](AsyncWebServerRequest *request) {
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
    });
}

void WebServerHandler::begin() {
    // Start AsyncWebServer
    server.begin();
    serverStarted = true;
    Serial.println("[WebServer] Web server started");
}

void WebServerHandler::end() {
    if (serverStarted) {
        server.end();
        serverStarted = false;
        Serial.println("[WebServer] Web server stopped");
    }
}

void WebServerHandler::handle() {
    // AsyncWebServer handles requests automatically
    // No need to call handle() for ESP32
}

bool WebServerHandler::isRunning() {
    return serverStarted;
}

AsyncWebServer* WebServerHandler::getServer() {
    return &server;
}



// Web page generation
String WebServerHandler::generateWebPage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>" + String(PROJECT_NAME) + " - Control Panel</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:0;padding:20px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#333;min-height:100vh}";
    html += ".container{max-width:1000px;margin:0 auto;background:rgba(255,255,255,0.95);border-radius:15px;box-shadow:0 20px 40px rgba(0,0,0,0.1);overflow:hidden}";
    html += ".header{background:linear-gradient(135deg,#2c3e50 0%,#34495e 100%);color:white;padding:30px;text-align:center}";
    html += ".header h1{font-size:2.5em;margin:0 0 10px 0;text-shadow:0 2px 4px rgba(0,0,0,0.3)}";
    html += ".content{padding:30px}";
    html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin-bottom:30px}";
    html += ".card{background:white;border-radius:10px;padding:25px;box-shadow:0 5px 15px rgba(0,0,0,0.1);border-left:4px solid #667eea}";
    html += ".card h3{color:#2c3e50;margin:0 0 15px 0;display:flex;align-items:center}";
    html += ".status-indicator{width:12px;height:12px;border-radius:50%;margin-right:10px}";
    html += ".status-connected{background-color:#27ae60}";
    html += ".status-disconnected{background-color:#e74c3c}";
    html += ".info-item{display:flex;justify-content:space-between;margin-bottom:10px;padding:8px 0;border-bottom:1px solid #ecf0f1}";
    html += ".info-item:last-child{border-bottom:none}";
    html += ".info-label{font-weight:600;color:#7f8c8d}";
    html += ".info-value{color:#2c3e50;font-weight:500}";
    html += ".btn{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;border:none;padding:12px 25px;border-radius:25px;cursor:pointer;font-size:14px;font-weight:600;margin:5px;transition:all 0.3s ease;text-decoration:none;display:inline-block}";
    html += ".btn:hover{transform:translateY(-2px);box-shadow:0 5px 15px rgba(102,126,234,0.4)}";
    html += ".btn-danger{background:linear-gradient(135deg,#e74c3c 0%,#c0392b 100%)}";
    html += ".actions{text-align:center;margin-top:30px}";
    html += ".footer{background:#34495e;color:white;text-align:center;padding:20px;font-size:0.9em}";
    html += "@media (max-width:768px){.grid{grid-template-columns:1fr}.header h1{font-size:2em}.content{padding:20px}}";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>" + String(PROJECT_NAME) + "</h1>";
    html += "<p>ESP32 Control Panel - Version " + String(PROJECT_VERSION) + "</p>";
    html += "</div>";
    
    html += "<div class='content'>";
    html += "<div class='grid'>";
    
    // WiFi Status Card
    html += "<div class='card'>";
    html += "<h3><span class='status-indicator' id='wifiStatus'></span>WiFi Status</h3>";
    html += "<div class='info-item'><span class='info-label'>Status:</span><span class='info-value' id='wifiConnected'>Loading...</span></div>";
    html += "<div class='info-item'><span class='info-label'>SSID:</span><span class='info-value' id='wifiSSID'>-</span></div>";
    html += "<div class='info-item'><span class='info-label'>IP Address:</span><span class='info-value' id='wifiIP'>-</span></div>";
    html += "<div class='info-item'><span class='info-label'>Signal:</span><span class='info-value' id='wifiRSSI'>-</span></div>";
    html += "</div>";
    
    // System Info Card
    html += "<div class='card'>";
    html += "<h3><span class='status-indicator status-connected'></span>System Information</h3>";
    html += "<div class='info-item'><span class='info-label'>Status:</span><span class='info-value' id='systemStatus'>Loading...</span></div>";
    html += "<div class='info-item'><span class='info-label'>Uptime:</span><span class='info-value' id='systemUptime'>-</span></div>";
    html += "<div class='info-item'><span class='info-label'>Free Memory:</span><span class='info-value' id='systemMemory'>-</span></div>";
    html += "<div class='info-item'><span class='info-label'>Chip ID:</span><span class='info-value' id='systemChipId'>-</span></div>";
    html += "</div>";
    
    // OTA Card
    html += "<div class='card'>";
    html += "<h3><span class='status-indicator status-connected'></span>OTA Updates</h3>";
    html += "<div class='info-item'><span class='info-label'>Status:</span><span class='info-value' id='otaEnabled'>Ready</span></div>";
    html += "<div class='info-item'><span class='info-label'>Update URL:</span><span class='info-value'><a href='/update' target='_blank' style='color:#667eea'>/update</a></span></div>";
    html += "<div class='info-item'><span class='info-label'>Username:</span><span class='info-value'>" + String(OTA_USERNAME) + "</span></div>";
    html += "</div>";
    
    html += "</div>";
    
    // Actions
    html += "<div class='actions'>";
    html += "<a href='/update' target='_blank' class='btn'>OTA Update</a>";
    html += "<button onclick='refreshData()' class='btn'>Refresh Data</button>";
    html += "<button onclick='restartDevice()' class='btn btn-danger'>Restart Device</button>";
    html += "<button onclick='resetWiFi()' class='btn btn-danger'>Reset WiFi</button>";
    html += "</div>";
    
    html += "</div>";
    
    html += "<div class='footer'>";
    html += "<p>&copy; 2025 " + String(PROJECT_AUTHOR) + " - " + String(PROJECT_NAME) + " v" + String(PROJECT_VERSION) + "</p>";
    html += "</div>";
    
    html += "</div>";
    
    // JavaScript
    html += "<script>";
    html += "function refreshData(){";
    html += "fetch('/api/status').then(r=>r.json()).then(d=>{";
    html += "document.getElementById('wifiStatus').className='status-indicator '+(d.wifi.connected?'status-connected':'status-disconnected');";
    html += "document.getElementById('wifiConnected').textContent=d.wifi.connected?'Connected':'Disconnected';";
    html += "document.getElementById('wifiSSID').textContent=d.wifi.ssid||'-';";
    html += "document.getElementById('wifiIP').textContent=d.wifi.ip||'-';";
    html += "document.getElementById('wifiRSSI').textContent=d.wifi.rssi?d.wifi.rssi+' dBm':'-';";
    html += "document.getElementById('systemStatus').textContent=d.status;";
    html += "document.getElementById('systemUptime').textContent=d.uptime;";
    html += "document.getElementById('systemMemory').textContent=(d.freeHeap/1024).toFixed(1)+' KB';";
    html += "document.getElementById('systemChipId').textContent='0x'+d.chipId.toString(16).toUpperCase();";
    html += "}).catch(e=>console.error('Error:',e));}";
    html += "function restartDevice(){if(confirm('Restart device?')){fetch('/api/restart',{method:'POST'}).then(r=>r.json()).then(d=>alert(d.message));}}";
    html += "function resetWiFi(){if(confirm('Reset WiFi settings? Device will restart.')){fetch('/api/reset',{method:'POST'}).then(r=>r.json()).then(d=>alert(d.message));}}";
    html += "setInterval(refreshData,30000);refreshData();";
    html += "</script>";
    
    html += "</body></html>";
    
    return html;
}
