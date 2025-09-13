#!/usr/bin/env python3
"""
Test script for analytics and remote diagnostics API endpoints
"""
import requests
import json
import time

# ESP32 IP address (you'll need to update this to your ESP32's IP)
ESP32_IP = "192.168.1.100"  # Update with your ESP32's IP
BASE_URL = f"http://{ESP32_IP}"

def test_endpoint(endpoint, description):
    """Test a single endpoint and print results"""
    try:
        print(f"\n{'='*60}")
        print(f"Testing: {description}")
        print(f"URL: {BASE_URL}{endpoint}")
        print(f"{'='*60}")
        
        response = requests.get(f"{BASE_URL}{endpoint}", timeout=10)
        
        if response.status_code == 200:
            data = response.json()
            print(f"✅ SUCCESS (Status: {response.status_code})")
            print(json.dumps(data, indent=2))
        else:
            print(f"❌ FAILED (Status: {response.status_code})")
            print(f"Response: {response.text}")
            
    except requests.exceptions.RequestException as e:
        print(f"❌ CONNECTION ERROR: {e}")
    except json.JSONDecodeError as e:
        print(f"❌ JSON DECODE ERROR: {e}")
        print(f"Raw response: {response.text[:500]}")

def main():
    """Main test function"""
    print("ESP32 Analytics and Diagnostics API Test")
    print(f"Target: {BASE_URL}")
    print(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Test basic sensor data first
    test_endpoint("/api/sensors", "Basic Sensor Data")
    
    # Test analytics endpoints
    test_endpoint("/api/analytics/statistics", "Analytics Statistics")
    test_endpoint("/api/analytics/trends", "Analytics Trends")
    test_endpoint("/api/analytics/prediction", "Analytics Prediction")
    test_endpoint("/api/analytics/outliers", "Analytics Outliers")
    test_endpoint("/api/analytics/report", "Analytics Report")
    
    # Test diagnostics endpoints
    test_endpoint("/api/diagnostics/system", "System Diagnostics")
    test_endpoint("/api/diagnostics/network", "Network Diagnostics")
    test_endpoint("/api/diagnostics/storage", "Storage Diagnostics")
    test_endpoint("/api/diagnostics/sensors", "Sensor Diagnostics")
    test_endpoint("/api/diagnostics/alerts", "Diagnostics Alerts")
    test_endpoint("/api/diagnostics/health", "Overall Health Score")
    
    # Test sensor health and info
    test_endpoint("/api/sensors/health", "Sensor Health")
    test_endpoint("/api/sensors/info", "Sensor Info")
    
    print(f"\n{'='*60}")
    print("Test completed!")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()