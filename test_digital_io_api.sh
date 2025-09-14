#!/bin/bash

# ESP32 Digital I/O API Test Script
# Tests all digital I/O endpoints with proper error handling

ESP32_IP="${1:-192.168.4.1}"
BASE_URL="http://$ESP32_IP"

echo "🔌 ESP32 Digital I/O API Test"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Target: $BASE_URL"
echo "Time: $(date)"
echo

# Test 1: Get all digital I/O status
echo "📊 Test 1: Get Digital I/O Status"
echo "Endpoint: GET /api/digital-io"
curl -s "$BASE_URL/api/digital-io" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 2: Get digital inputs only  
echo "📥 Test 2: Get Digital Inputs"
echo "Endpoint: GET /api/digital-io/inputs"
curl -s "$BASE_URL/api/digital-io/inputs" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 3: Get digital outputs only
echo "📤 Test 3: Get Digital Outputs"
echo "Endpoint: GET /api/digital-io/outputs"
curl -s "$BASE_URL/api/digital-io/outputs" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 4: Control Digital Output 0 - Turn ON
echo "🔴 Test 4: Turn Output 0 ON"
echo "Endpoint: POST /api/digital-io/output?output=0&state=ON"
curl -s -X POST "$BASE_URL/api/digital-io/output?output=0&state=ON" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Wait a moment
sleep 2

# Test 5: Control Digital Output 0 - Turn OFF
echo "⚫ Test 5: Turn Output 0 OFF"
echo "Endpoint: POST /api/digital-io/output?output=0&state=OFF"
curl -s -X POST "$BASE_URL/api/digital-io/output?output=0&state=OFF" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 6: Pulse Output 1 for 2 seconds
echo "⚡ Test 6: Pulse Output 1 (2 seconds)"
echo "Endpoint: POST /api/digital-io/output?output=1&state=PULSE&duration=2000"
curl -s -X POST "$BASE_URL/api/digital-io/output?output=1&state=PULSE&duration=2000" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 7: Blink Output 2
echo "💫 Test 7: Blink Output 2"
echo "Endpoint: POST /api/digital-io/output?output=2&state=BLINK"
curl -s -X POST "$BASE_URL/api/digital-io/output?output=2&state=BLINK" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 8: Turn off blinking output
sleep 3
echo "🛑 Test 8: Stop Blinking Output 2"
echo "Endpoint: POST /api/digital-io/output?output=2&state=OFF"
curl -s -X POST "$BASE_URL/api/digital-io/output?output=2&state=OFF" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 9: Error handling - Invalid output index
echo "❌ Test 9: Invalid Output Index"
echo "Endpoint: POST /api/digital-io/output?output=5&state=ON"
curl -s -X POST "$BASE_URL/api/digital-io/output?output=5&state=ON" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 10: Error handling - Invalid state
echo "❌ Test 10: Invalid State"
echo "Endpoint: POST /api/digital-io/output?output=0&state=INVALID"
curl -s -X POST "$BASE_URL/api/digital-io/output?output=0&state=INVALID" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 11: Error handling - Missing parameters
echo "❌ Test 11: Missing Parameters"
echo "Endpoint: POST /api/digital-io/output?output=0"
curl -s -X POST "$BASE_URL/api/digital-io/output?output=0" | jq '.' || echo "❌ Failed"
echo -e "\n"

# Test 12: Final status check
echo "📋 Test 12: Final Status Check"
echo "Endpoint: GET /api/digital-io"
curl -s "$BASE_URL/api/digital-io" | jq '{
  initialized: .initialized,
  total_updates: .total_updates,
  inputs: [.inputs[] | {index: .index, name: .name, state: .state, pulse_count: .pulse_count}],
  outputs: [.outputs[] | {index: .index, name: .name, state: .state, is_on: .is_on}]
}' || echo "❌ Failed"
echo -e "\n"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Digital I/O API tests completed!"
echo "Note: Physical digital inputs will show current hardware state"
echo "Note: Outputs 0-3 correspond to DO1-DO4 pins on ESP32"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"