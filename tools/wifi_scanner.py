#!/usr/bin/env python3
"""
ESP32 WiFi Scanner
-------------------
Tool to scan available WiFi networks on ESP32 and analyze signal strength.

Usage:
    python wifi_scanner.py [options]

Options:
    --port PORT      Serial port (auto-detects if not specified)
    --baud RATE      Baud rate (default: 115200)
    --scan-time SEC  Network scan duration in seconds (default: 5)
    --export FILE    Export results to CSV file
    --format FORMAT  Output format: table, json, or csv (default: table)
    --sort FIELD     Sort by: rssi, ssid, channel, security (default: rssi)
    --help           Show this help message

Example:
    python wifi_scanner.py --port /dev/ttyUSB0 --export wifi_scan.csv
"""

import os
import sys
import argparse
import serial
import serial.tools.list_ports
import json
import csv
import time
import platform
import re
from datetime import datetime
from pathlib import Path

# ANSI color codes for pretty output
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    CYAN = '\033[96m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description='ESP32 WiFi Scanner')
    parser.add_argument('--port', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--scan-time', type=int, default=5, help='Network scan duration in seconds')
    parser.add_argument('--export', help='Export results to CSV file')
    parser.add_argument('--format', choices=['table', 'json', 'csv'], default='table', help='Output format')
    parser.add_argument('--sort', choices=['rssi', 'ssid', 'channel', 'security'], default='rssi', help='Sort field')
    
    return parser.parse_args()

def find_serial_port():
    """Auto-detect serial port."""
    system = platform.system()
    
    if system == 'Windows':
        # Check for common ESP32 usb-serial devices on Windows
        ports = list(serial.tools.list_ports.comports())
        for port in ports:
            if 'CP210x' in port.description or 'CH340' in port.description:
                return port.device
        # Fallback to a common port
        return 'COM3'
    
    elif system == 'Darwin':  # macOS
        # Common macOS ports for ESP32
        common_ports = [
            '/dev/tty.SLAB_USBtoUART',
            '/dev/tty.wchusbserial1410',
            '/dev/tty.usbserial-0001',
            '/dev/tty.usbserial-A50285BI'
        ]
        for port in common_ports:
            if os.path.exists(port):
                return port
        # Check for cu.* devices
        import glob
        ports = glob.glob('/dev/cu.usbserial*')
        if ports:
            return ports[0]
        
        return '/dev/cu.SLAB_USBtoUART'  # Default fallback
    
    else:  # Linux and others
        # Common Linux ports for ESP32
        common_ports = ['/dev/ttyUSB0', '/dev/ttyACM0']
        for port in common_ports:
            if os.path.exists(port):
                return port
        
        return '/dev/ttyUSB0'  # Default fallback

def security_type_name(encryption_type):
    """Convert encryption type number to readable name."""
    types = {
        0: "OPEN",
        1: "WEP",
        2: "WPA",
        3: "WPA2",
        4: "WPA_WPA2",
        5: "WPA2_ENTERPRISE",
        6: "WPA3",
        7: "WPA2_WPA3",
        8: "WAPI"
    }
    return types.get(encryption_type, f"Unknown({encryption_type})")

def signal_quality(rssi):
    """Convert RSSI to signal quality description."""
    if rssi >= -50:
        return "Excellent"
    elif rssi >= -60:
        return "Good"
    elif rssi >= -70:
        return "Fair"
    elif rssi >= -80:
        return "Poor"
    else:
        return "Very Poor"

def signal_bars(rssi):
    """Convert RSSI to signal bars representation."""
    if rssi >= -55:
        return "█████"  # 5 bars
    elif rssi >= -65:
        return "████ "  # 4 bars
    elif rssi >= -70:
        return "███  "  # 3 bars
    elif rssi >= -80:
        return "██   "  # 2 bars
    else:
        return "█    "  # 1 bar

def create_wifi_scan_sketch():
    """Create a temporary Arduino sketch for WiFi scanning."""
    sketch = """
// ESP32 WiFi Scanner

#include <ESP32WiFi.h>
#include <ArduinoJson.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Disable WiFi persistence to save flash
  WiFi.persistent(false);
  
  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("ESP32_WIFI_SCANNER_READY");
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\\n');
    command.trim();
    
    if (command == "SCAN") {
      scanNetworks();
    }
  }
  delay(100);
}

void scanNetworks() {
  Serial.println("ESP32_WIFI_SCAN_START");
  
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    Serial.println("ESP32_WIFI_SCAN_COMPLETE");
    Serial.println("No networks found");
  } else {
    // JSON array for all networks
    Serial.println("ESP32_WIFI_SCAN_RESULTS");
    
    for (int i = 0; i < n; ++i) {
      // Create JSON object for this network
      StaticJsonDocument<256> doc;
      doc["ssid"] = WiFi.SSID(i);
      doc["rssi"] = WiFi.RSSI(i);
      doc["channel"] = WiFi.channel(i);
      doc["encryption_type"] = WiFi.encryptionType(i);
      doc["bssid"] = WiFi.BSSIDstr(i);
      doc["hidden"] = WiFi.isHidden(i);
      
      // Serialize JSON to Serial
      serializeJson(doc, Serial);
      Serial.println();
      
      // Small delay to prevent buffer overflow
      delay(10);
    }
    
    Serial.println("ESP32_WIFI_SCAN_COMPLETE");
  }
}
"""
    
    # Create a temporary file
    temp_dir = Path.cwd() / "temp"
    temp_dir.mkdir(exist_ok=True)
    
    sketch_file = temp_dir / "wifi_scanner.ino"
    with open(sketch_file, "w") as f:
        f.write(sketch)
    
    return sketch_file

def upload_sketch(port, sketch_file):
    """Upload the sketch to ESP32."""
    print(f"{Colors.BLUE}Uploading WiFi scanner sketch to ESP32...{Colors.ENDC}")
    
    # Construct the Arduino CLI command
    try:
        import subprocess
        result = subprocess.run([
            "pio", "run", 
            "--target", "upload",
            "--upload-port", port
        ], capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"{Colors.RED}Error uploading sketch: {result.stderr}{Colors.ENDC}")
            return False
        
        print(f"{Colors.GREEN}Sketch uploaded successfully!{Colors.ENDC}")
        return True
            
    except Exception as e:
        print(f"{Colors.RED}Error uploading sketch: {str(e)}{Colors.ENDC}")
        return False

def scan_networks(port, baud_rate, scan_time=5):
    """Scan WiFi networks using ESP32."""
    networks = []
    
    try:
        with serial.Serial(port, baud_rate, timeout=1) as ser:
            print(f"{Colors.BLUE}Connecting to ESP32...{Colors.ENDC}")
            
            # Wait for ready message
            deadline = time.time() + 10
            ready = False
            
            while time.time() < deadline:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if "ESP32_WIFI_SCANNER_READY" in line:
                    ready = True
                    break
            
            if not ready:
                print(f"{Colors.YELLOW}Device not ready. Please reset your ESP32.{Colors.ENDC}")
                return None
                
            # Send scan command
            print(f"{Colors.BLUE}Starting WiFi scan (this will take ~{scan_time} seconds)...{Colors.ENDC}")
            ser.write(b"SCAN\n")
            
            # Wait for scan to start
            scan_started = False
            deadline = time.time() + 5
            
            while time.time() < deadline:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if "ESP32_WIFI_SCAN_START" in line:
                    scan_started = True
                    break
            
            if not scan_started:
                print(f"{Colors.RED}Scan failed to start.{Colors.ENDC}")
                return None
            
            # Wait for results
            results_started = False
            scan_complete = False
            deadline = time.time() + scan_time + 10
            
            while time.time() < deadline:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                
                if "ESP32_WIFI_SCAN_RESULTS" in line:
                    results_started = True
                    continue
                    
                if "ESP32_WIFI_SCAN_COMPLETE" in line:
                    scan_complete = True
                    break
                
                if results_started and line:
                    try:
                        network = json.loads(line)
                        networks.append(network)
                    except json.JSONDecodeError:
                        pass
            
            if not scan_complete:
                print(f"{Colors.YELLOW}Scan timed out. Partial results may be available.{Colors.ENDC}")
            
            return networks
                
    except serial.SerialException as e:
        print(f"{Colors.RED}Serial error: {str(e)}{Colors.ENDC}")
        return None
    except Exception as e:
        print(f"{Colors.RED}Error: {str(e)}{Colors.ENDC}")
        return None

def print_table(networks, sort_by='rssi'):
    """Print network information as a formatted table."""
    if not networks:
        print(f"{Colors.YELLOW}No networks found.{Colors.ENDC}")
        return
    
    # Sort networks
    if sort_by == 'rssi':
        networks.sort(key=lambda x: x['rssi'], reverse=True)
    elif sort_by == 'ssid':
        networks.sort(key=lambda x: x['ssid'])
    elif sort_by == 'channel':
        networks.sort(key=lambda x: x['channel'])
    elif sort_by == 'security':
        networks.sort(key=lambda x: x['encryption_type'])
    
    # Print header
    print(f"\n{Colors.BOLD}{'SSID':<32} {'Signal':<10} {'Ch':<4} {'Security':<14} {'BSSID':<18} {'Quality'}{Colors.ENDC}")
    print("-" * 92)
    
    # Print networks
    for network in networks:
        ssid = network['ssid'] or f"{Colors.YELLOW}[Hidden]{Colors.ENDC}"
        if len(ssid) > 30:
            ssid = ssid[:27] + "..."
            
        quality = signal_quality(network['rssi'])
        bars = signal_bars(network['rssi'])
        
        # Color the signal strength
        if network['rssi'] >= -60:
            signal = f"{Colors.GREEN}{network['rssi']} dBm {bars}{Colors.ENDC}"
        elif network['rssi'] >= -70:
            signal = f"{Colors.YELLOW}{network['rssi']} dBm {bars}{Colors.ENDC}"
        else:
            signal = f"{Colors.RED}{network['rssi']} dBm {bars}{Colors.ENDC}"
            
        security = security_type_name(network['encryption_type'])
        
        print(f"{ssid:<32} {signal:<28} {network['channel']:<4} {security:<14} {network['bssid']:<18} {quality}")
    
    print(f"\n{Colors.BOLD}Total networks found: {len(networks)}{Colors.ENDC}")

def export_to_csv(networks, filename):
    """Export network information to CSV file."""
    if not networks:
        print(f"{Colors.YELLOW}No networks to export.{Colors.ENDC}")
        return
    
    try:
        with open(filename, 'w', newline='') as f:
            fieldnames = [
                'ssid', 'rssi', 'quality', 'channel', 
                'security_type', 'bssid', 'hidden'
            ]
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            
            writer.writeheader()
            for network in networks:
                writer.writerow({
                    'ssid': network['ssid'],
                    'rssi': network['rssi'],
                    'quality': signal_quality(network['rssi']),
                    'channel': network['channel'],
                    'security_type': security_type_name(network['encryption_type']),
                    'bssid': network['bssid'],
                    'hidden': network['hidden']
                })
                
        print(f"{Colors.GREEN}Exported to {filename}{Colors.ENDC}")
    except Exception as e:
        print(f"{Colors.RED}Error exporting to CSV: {str(e)}{Colors.ENDC}")

def main():
    args = parse_arguments()
    
    # Set up serial port
    port = args.port
    if not port:
        port = find_serial_port()
        print(f"{Colors.BLUE}Auto-detected serial port: {port}{Colors.ENDC}")
    
    baud_rate = args.baud
    
    print(f"{Colors.HEADER}{Colors.BOLD}ESP32 WiFi Scanner{Colors.ENDC}")
    print("=" * 50)
    
    # Scan for networks
    networks = scan_networks(port, baud_rate, args.scan_time)
    
    if not networks:
        print(f"{Colors.RED}No network data received.{Colors.ENDC}")
        sys.exit(1)
    
    # Display or export results
    if args.format == 'table':
        print_table(networks, args.sort)
    elif args.format == 'json':
        print(json.dumps(networks, indent=2))
    elif args.format == 'csv':
        for network in networks:
            print(f"{network['ssid']},{network['rssi']},{network['channel']},{security_type_name(network['encryption_type'])},{network['bssid']}")
    
    # Export to CSV if requested
    if args.export:
        export_to_csv(networks, args.export)

if __name__ == '__main__':
    main()
