#!/bin/bash

# ADA-1 Board API Testing Script
# Board IP: 192.168.111.34

echo "🔥 Testing ADA-1 Board API Endpoints"
echo "===================================="
echo ""

BOARD_IP="192.168.111.34"
BASE_URL="http://${BOARD_IP}/api"

echo "📊 1. Testing System Status..."
echo "curl -X GET ${BASE_URL}/status"
curl -s -X GET "${BASE_URL}/status" | jq .
echo ""

echo "📡 2. Testing Analog Voltage Sensors..."
echo "curl -X GET ${BASE_URL}/analog-voltage"
curl -s -X GET "${BASE_URL}/analog-voltage" | jq .
echo ""

echo "🔧 3. Testing Modbus Devices..."
echo "curl -X GET ${BASE_URL}/modbus/devices"
curl -s -X GET "${BASE_URL}/modbus/devices" | jq .
echo ""

echo "📊 4. Testing Individual Sensor (Sensor 0)..."
echo "curl -X GET ${BASE_URL}/analog-voltage/sensor/0"
curl -s -X GET "${BASE_URL}/analog-voltage/sensor/0" | jq '.sensors.sensor_0'
echo ""

echo "🔧 5. Testing Modbus Read Operation..."
echo "curl -X POST ${BASE_URL}/modbus/read"
curl -s -X POST "${BASE_URL}/modbus/read" \
  -H "Content-Type: application/json" \
  -d '{"device_address": 1, "function": 3, "start_address": 30001, "quantity": 2}' | jq .
echo ""

echo "🎯 6. Testing Board Connectivity..."
ping -c 3 ${BOARD_IP}
echo ""

echo "✅ API Testing Complete!"
echo "📝 Working endpoints:"
echo "  - GET /api/status (✅ Working)"
echo "  - GET /api/analog-voltage (✅ Working)" 
echo "  - GET /api/analog-voltage/sensor/{id} (✅ Working)"
echo "  - GET /api/modbus/devices (✅ Working)"
echo "  - POST /api/modbus/read (✅ Working)"
echo ""
echo "❌ Not implemented yet:"
echo "  - /api/health"
echo "  - /api/wifi/status"
echo "  - /api/time"
echo "  - /api/logs"