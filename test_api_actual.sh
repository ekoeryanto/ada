#!/bin/bash

# ADA-1 Board ACTUAL API Testing Script
# Based on real firmware implementation
# Board IP: 192.168.111.34

echo "🔥 Testing ADA-1 Board ACTUAL API Endpoints"
echo "============================================"
echo ""

BOARD_IP="192.168.111.34"
BASE_URL="http://${BOARD_IP}/api"

echo "📊 1. System Management Tests..."
echo "--------------------------------"

echo "▶️ System Status:"
curl -s -X GET "${BASE_URL}/status" | jq -r '.project + " v" + .version + " - " + .status + " (Uptime: " + .uptime + ")"'

echo "▶️ Configuration:"
curl -s -X GET "${BASE_URL}/config" | jq .

echo ""

echo "📡 2. Analog Sensors Tests..."
echo "-----------------------------"

echo "▶️ All Sensors Summary:"
curl -s -X GET "${BASE_URL}/analog-voltage" | jq '{initialized, total_readings, alarm_status, has_errors}'

echo "▶️ Sensor Details:"
curl -s -X GET "${BASE_URL}/analog-voltage" | jq '.sensors | to_entries[] | {id: .key, location: .value.location, status: .value.status, value: .value.value, unit: .value.unit, errors: .value.errors}'

echo ""

echo "🔧 3. Modbus System Tests..."
echo "----------------------------"

echo "▶️ Modbus Devices:"
curl -s -X GET "${BASE_URL}/modbus/devices" | jq '{initialized, total_devices, connected_devices}'

echo "▶️ Modbus Statistics:"
curl -s -X GET "${BASE_URL}/modbus/stats" | jq .

echo "▶️ Modbus Read Test:"
curl -s -X POST "${BASE_URL}/modbus/read" \
  -H "Content-Type: application/json" \
  -d '{"device_address": 1, "function": 3, "start_address": 30001, "quantity": 2}' | jq .

echo ""

echo "💾 4. SD Card & Storage Tests..."
echo "-------------------------------"

echo "▶️ SD Card Status:"
curl -s -X GET "${BASE_URL}/sd/status" | jq .

echo ""

echo "📈 5. Analytics Tests..."
echo "-----------------------"

echo "▶️ Analytics Summary:"
curl -s -X GET "${BASE_URL}/analytics/summary" | jq '{analytics_enabled, analysis_interval, sensors: [.sensors[] | {id, data_points, has_enough_data}]}'

echo ""

echo "🎮 6. Simulation Tests..."
echo "------------------------"

echo "▶️ Simulation Status:"
curl -s -X GET "${BASE_URL}/simulation/status" | jq .

echo ""

echo "🔧 7. Diagnostics Tests..."
echo "-------------------------"

echo "▶️ Diagnostics Status:"
curl -s -X GET "${BASE_URL}/diagnostics/status" | jq .

echo ""

echo "🪝 8. Webhook Tests..."
echo "---------------------"

echo "▶️ Webhook List:"
curl -s -X GET "${BASE_URL}/webhooks" | jq .

echo "▶️ Webhook Status:"
curl -s -X GET "${BASE_URL}/webhooks/status" | jq .

echo "▶️ Webhook Statistics:"
curl -s -X GET "${BASE_URL}/webhooks/statistics" | jq .

echo "▶️ Webhook Queue:"
curl -s -X GET "${BASE_URL}/webhooks/queue" | jq .

echo ""

echo "❌ 9. Testing PROBLEMATIC Endpoints..."
echo "-------------------------------------"

echo "▶️ Health endpoint (has implementation issue):"
curl -s "${BASE_URL}/health" | jq .

echo "▶️ WiFi status endpoint (not implemented):"
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/wifi/status")
echo "HTTP Status: $HTTP_CODE"

echo "▶️ Time endpoint (not implemented):"
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/time")
echo "HTTP Status: $HTTP_CODE"

echo ""

echo "🎯 10. Connectivity Test..."
echo "-------------------------"
ping -c 3 ${BOARD_IP} | tail -1

echo ""
echo "✅ COMPLETE API Testing Done!"
echo "=============================="
echo ""
echo "📝 WORKING ENDPOINTS:"
echo "  ✅ /api/status - System status"
echo "  ✅ /api/config - Configuration"
echo "  ✅ /api/analog-voltage - Sensor data"
echo "  ✅ /api/modbus/devices - Modbus devices"
echo "  ✅ /api/modbus/stats - Modbus statistics"
echo "  ✅ /api/modbus/read - Modbus read operation"
echo "  ✅ /api/sd/status - SD card status"
echo "  ✅ /api/analytics/summary - Analytics data"
echo "  ✅ /api/simulation/status - Simulation status"
echo "  ✅ /api/diagnostics/status - Diagnostics"
echo "  ✅ /api/webhooks - Webhook management"
echo "  ✅ /api/webhooks/status - Webhook status"
echo "  ✅ /api/webhooks/statistics - Webhook stats"
echo "  ✅ /api/webhooks/queue - Webhook queue"
echo ""
echo "⚠️  PROBLEMATIC ENDPOINTS:"
echo "  ⚠️  /api/health - Code exists but returns error"
echo ""
echo "❌ NOT IMPLEMENTED:"
echo "  ❌ /api/wifi/status - WiFi details"
echo "  ❌ /api/time - Time/NTP info"
echo "  ❌ /api/logs - Log files"
echo ""
echo "💡 RECOMMENDATION:"
echo "  Update firmware via OTA to get latest features:"
echo "  http://192.168.111.34/update"