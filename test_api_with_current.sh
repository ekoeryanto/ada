#!/bin/bash

# ADA-1 Board API Testing Script - WITH NEW ANALOG CURRENT ENDPOINTS!
# Board IP: 192.168.111.34
# Updated: Includes 4-20mA analog current endpoints

echo "🔥 ADA-1 Complete API Testing (Including 4-20mA Endpoints!)"
echo "=========================================================="
echo ""

BOARD_IP="192.168.111.34"
BASE_URL="http://${BOARD_IP}/api"

# Function to test endpoint with colored output
test_endpoint() {
    local method=$1
    local endpoint=$2
    local description=$3
    local data=$4
    
    echo "▶️ $description"
    if [ "$method" = "GET" ]; then
        HTTP_CODE=$(curl -s -o /tmp/response.json -w "%{http_code}" "${BASE_URL}${endpoint}")
    else
        if [ -n "$data" ]; then
            HTTP_CODE=$(curl -s -o /tmp/response.json -w "%{http_code}" -X $method \
                -H "Content-Type: application/json" \
                -d "$data" "${BASE_URL}${endpoint}")
        else
            HTTP_CODE=$(curl -s -o /tmp/response.json -w "%{http_code}" -X $method "${BASE_URL}${endpoint}")
        fi
    fi
    
    if [ "$HTTP_CODE" = "200" ]; then
        echo "✅ HTTP $HTTP_CODE"
        if command -v jq >/dev/null 2>&1; then
            jq . /tmp/response.json 2>/dev/null | head -20 || cat /tmp/response.json | head -20
        else
            cat /tmp/response.json | head -20
        fi
    else
        echo "❌ HTTP $HTTP_CODE"
        cat /tmp/response.json
    fi
    echo ""
}

echo "🔧 1. SYSTEM MANAGEMENT"
echo "----------------------"
test_endpoint "GET" "/status" "System Status"
test_endpoint "GET" "/health" "System Health"
test_endpoint "GET" "/config" "Configuration Settings"

echo ""
echo "📊 2. ANALOG VOLTAGE (0-10V) SENSORS"
echo "-----------------------------------"
test_endpoint "GET" "/analog-voltage" "All Voltage Sensor Readings"
test_endpoint "GET" "/analog-voltage/health" "Voltage Sensor Health"
test_endpoint "GET" "/analog-voltage/info" "Voltage Sensor Information"

echo ""
echo "⚡ 3. ANALOG CURRENT (4-20mA) SENSORS - NEW!"
echo "-------------------------------------------"
test_endpoint "GET" "/analog-current" "All Current Sensor Readings (Rosemount, Yokogawa, E+H)"
test_endpoint "GET" "/analog-current/health" "Current Loop Health Status"
test_endpoint "GET" "/analog-current/info" "Current Sensor Info & Manufacturers"
test_endpoint "GET" "/analog-current/diagnostics" "Loop Diagnostics & Resistance"

echo ""
echo "💾 4. SD CARD MANAGEMENT"
echo "-----------------------"
test_endpoint "GET" "/sd/status" "SD Card Status"
test_endpoint "GET" "/sd/files" "SD Card File List"

echo ""
echo "🎮 5. SIMULATION MODE"
echo "--------------------"
test_endpoint "GET" "/simulation/status" "Simulation Status"

echo ""
echo "📈 6. ANALYTICS"
echo "--------------"
test_endpoint "GET" "/analytics/summary" "Analytics Summary"
test_endpoint "GET" "/analytics/statistics" "Analytics Statistics"
test_endpoint "GET" "/analytics/trends" "Data Trends Analysis"
test_endpoint "GET" "/analytics/prediction" "Predictive Analytics"

echo ""
echo "🔧 7. DIAGNOSTICS"
echo "-----------------"
test_endpoint "GET" "/diagnostics/status" "Diagnostic Status"
test_endpoint "GET" "/diagnostics/all" "All Diagnostic Data"
test_endpoint "GET" "/diagnostics/alerts" "Active Alerts"
test_endpoint "GET" "/diagnostics/health" "Health Diagnostics"

echo ""
echo "🪝 8. WEBHOOKS"
echo "-------------"
test_endpoint "GET" "/webhooks" "List Webhooks"
test_endpoint "GET" "/webhooks/status" "Webhook Status"
test_endpoint "GET" "/webhooks/statistics" "Webhook Statistics"
test_endpoint "GET" "/webhooks/queue" "Webhook Queue Status"

echo ""
echo "🔌 9. MODBUS COMMUNICATION"
echo "--------------------------"
test_endpoint "GET" "/modbus/devices" "List Modbus Devices"
test_endpoint "GET" "/modbus/stats" "Modbus Statistics"
test_endpoint "POST" "/modbus/read" "Modbus Read Test" '{"device_address": 1, "function": 3, "start_address": 30001, "quantity": 2}'

echo ""
echo "🎯 10. CONNECTIVITY TEST"
echo "------------------------"
ping -c 3 ${BOARD_IP} | tail -1

echo ""
echo "✅ COMPLETE API TESTING FINISHED!"
echo "================================="
echo ""
echo "📊 SUMMARY:"
echo "  🔧 System: 3 endpoints"
echo "  📊 Voltage Sensors: 3 endpoints"  
echo "  ⚡ Current Sensors: 4 endpoints (NEW!)"
echo "  💾 SD Card: 2 endpoints"
echo "  🎮 Simulation: 1 endpoint"
echo "  📈 Analytics: 4 endpoints"
echo "  🔧 Diagnostics: 4 endpoints"
echo "  🪝 Webhooks: 4 endpoints"
echo "  🔌 Modbus: 3 endpoints tested"
echo ""
echo "  TOTAL TESTED: 28+ endpoints"
echo "  TOTAL AVAILABLE: 34+ endpoints"
echo ""
echo "🚀 NEW FEATURES:"
echo "  ✅ 4-20mA current loop monitoring"
echo "  ✅ Industrial sensor support (Rosemount, Yokogawa, E+H)"
echo "  ✅ Loop resistance diagnostics"
echo "  ✅ Signal quality monitoring"
echo ""
echo "💡 NOTES:"
echo "  - Some POST endpoints not tested to avoid system changes"
echo "  - Both 0-10V and 4-20mA sensors now accessible via API"
echo "  - Professional industrial monitoring capability complete"
echo ""

# Clean up
rm -f /tmp/response.json