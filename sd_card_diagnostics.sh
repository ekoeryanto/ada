#!/bin/bash

# ESP32 SD Card Troubleshooting Script
# Check SD card status and potential issues

ESP32_IP="${1:-192.168.111.34}"
BASE_URL="http://$ESP32_IP"

echo "🔍 ESP32 SD Card Diagnostics"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Target: $BASE_URL"
echo "Time: $(date)"
echo

# Test 1: SD Card Basic Status
echo "💾 Test 1: SD Card Basic Status"
echo "Endpoint: GET /api/sd/status"
curl -s "$BASE_URL/api/sd/status" | jq '.'
echo -e "\n"

# Test 2: System SD Status
echo "📊 Test 2: System SD Status"  
echo "Endpoint: GET /api/status (sd section)"
curl -s "$BASE_URL/api/status" | jq '.sd'
echo -e "\n"

# Test 3: Health Check SD Status
echo "🏥 Test 3: Health Check SD Status"
echo "Endpoint: GET /api/health"
curl -s "$BASE_URL/api/health" | jq '.sd'
echo -e "\n"

# Test 4: Storage Alerts
echo "⚠️ Test 4: Storage-Related Alerts"
echo "Endpoint: GET /api/diagnostics/alerts"
curl -s "$BASE_URL/api/diagnostics/alerts" | jq '.alerts[] | select(.category == "storage")'
echo -e "\n"

# Test 5: SD Card Test Function
echo "🧪 Test 5: SD Card Test Function"
echo "Endpoint: POST /api/sd/test"
curl -s -X POST "$BASE_URL/api/sd/test" | jq '.'
echo -e "\n"

# Test 6: File List Attempt
echo "📁 Test 6: File List Attempt"
echo "Endpoint: GET /api/sd/files"
curl -s "$BASE_URL/api/sd/files" | jq '.'
echo -e "\n"

# Test 7: Storage Diagnostics
echo "🔧 Test 7: Storage Diagnostics"
echo "Endpoint: GET /api/diagnostics/data (storage section)"
curl -s "$BASE_URL/api/diagnostics/data" | jq '.storage // "No storage data available"'
echo -e "\n"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📋 DIAGNOSIS SUMMARY"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo
echo "🔍 POSSIBLE CAUSES FOR SD CARD FAILURE:"
echo "1. 🔌 Hardware Connection Issues:"
echo "   - SD card not properly inserted"
echo "   - Loose connection in SD card slot"
echo "   - Damaged SD card slot or pins"
echo
echo "2. ⚡ Power Supply Issues:"
echo "   - Insufficient power to SD card"
echo "   - Voltage drops during initialization"
echo "   - Power supply noise affecting SPI communication"
echo
echo "3. 💽 SD Card Issues:"
echo "   - SD card corrupted or damaged"
echo "   - Unsupported SD card type (needs FAT32)"
echo "   - SD card not formatted properly"
echo "   - Write protection enabled"
echo
echo "4. 🔧 Pin Configuration Issues:"
echo "   - SPI pins (CS=5, MOSI=23, MISO=19, SCK=18) conflict"
echo "   - Pin already in use by other peripherals"
echo "   - Incorrect pin assignment in firmware"
echo
echo "5. 📡 SPI Communication Issues:"
echo "   - SPI bus conflicts with other devices"
echo "   - SPI speed too high for SD card"
echo "   - Signal integrity issues (long wires, noise)"
echo
echo "🛠️ TROUBLESHOOTING STEPS:"
echo "1. Check physical SD card insertion"
echo "2. Try a different SD card (FAT32 formatted, <32GB)"
echo "3. Check power supply voltage (should be stable 3.3V/5V)"
echo "4. Verify pin connections match firmware config"
echo "5. Try lower SPI speed (modify firmware if needed)"
echo "6. Check for pin conflicts with other peripherals"
echo
echo "💡 QUICK FIX ATTEMPTS:"
echo "1. Remove and reinsert SD card"
echo "2. Format SD card as FAT32 on computer"
echo "3. Try a different SD card"
echo "4. Check ESP32 power supply stability"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"