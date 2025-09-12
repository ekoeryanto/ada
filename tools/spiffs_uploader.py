#!/usr/bin/env python3
"""
ESP32 SPIFFS File Uploader
---------------------------
Tool to package and upload files to the ESP32's SPIFFS filesystem.

Usage:
    python spiffs_uploader.py [options]

Options:
    --data DIR       Path to data directory (default: data/)
    --upload         Upload to device after building
    --port PORT      Serial port for upload
    --ip IP          IP address for OTA upload
    --clean          Clean the data directory before building
    --help           Show this help message

Example:
    python spiffs_uploader.py --data ./data --upload --port /dev/ttyUSB0
"""

import os
import sys
import re
import subprocess
import argparse
import shutil
from pathlib import Path
import platform

# ANSI color codes for pretty output
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description='ESP32 SPIFFS File Uploader')
    parser.add_argument('--data', default='data', help='Path to data directory')
    parser.add_argument('--upload', action='store_true', help='Upload to device after building')
    parser.add_argument('--port', help='Serial port for upload')
    parser.add_argument('--ip', help='IP address for OTA upload')
    parser.add_argument('--clean', action='store_true', help='Clean data directory before building')
    
    return parser.parse_args()

def find_serial_port():
    """Auto-detect serial port based on platform."""
    system = platform.system()
    
    if system == 'Windows':
        # Check for common ESP32 usb-serial devices on Windows
        import serial.tools.list_ports
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

def calculate_directory_size(directory):
    """Calculate the size of a directory and its contents."""
    total_size = 0
    for dirpath, dirnames, filenames in os.walk(directory):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            if not os.path.islink(fp):
                total_size += os.path.getsize(fp)
    
    return total_size

def format_size(size_bytes):
    """Format size in human-readable format."""
    if size_bytes < 1024:
        return f"{size_bytes} B"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes / 1024:.2f} KB"
    else:
        return f"{size_bytes / (1024 * 1024):.2f} MB"

def check_data_directory(data_dir):
    """Check if data directory exists and contains files."""
    data_path = Path(data_dir)
    
    if not data_path.exists():
        print(f"{Colors.YELLOW}Data directory not found, creating: {data_dir}{Colors.ENDC}")
        os.makedirs(data_dir)
        return False
    
    if not any(data_path.iterdir()):
        print(f"{Colors.YELLOW}Data directory is empty: {data_dir}{Colors.ENDC}")
        return False
    
    size = calculate_directory_size(data_dir)
    file_count = sum(1 for _ in data_path.glob('**/*') if _.is_file())
    
    print(f"{Colors.GREEN}Data directory: {data_dir}{Colors.ENDC}")
    print(f"  - {file_count} files")
    print(f"  - {format_size(size)} total size")
    
    return True

def build_spiffs_binary(data_dir, output_dir='.pio/build/d1_mini'):
    """Build SPIFFS binary from data directory."""
    # Ensure output directory exists
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    # Path to the output binary
    spiffs_bin = os.path.join(output_dir, 'spiffs.bin')
    
    print(f"{Colors.BLUE}Building SPIFFS image...{Colors.ENDC}")
    
    # Use PlatformIO's built-in command for building SPIFFS
    try:
        result = subprocess.run(
            ['pio', 'run', '--target', 'buildfs'],
            capture_output=True, 
            text=True
        )
        
        if result.returncode != 0:
            print(f"{Colors.RED}Error building SPIFFS image:{Colors.ENDC}")
            print(result.stderr)
            return None
        
        # Check if file was created
        if os.path.exists(spiffs_bin):
            size = os.path.getsize(spiffs_bin)
            print(f"{Colors.GREEN}SPIFFS image built successfully: {format_size(size)}{Colors.ENDC}")
            return spiffs_bin
        else:
            print(f"{Colors.RED}SPIFFS binary not found after build{Colors.ENDC}")
            return None
            
    except Exception as e:
        print(f"{Colors.RED}Error building SPIFFS: {str(e)}{Colors.ENDC}")
        return None

def upload_spiffs(port=None, ip=None):
    """Upload SPIFFS binary to the device."""
    upload_cmd = ['pio', 'run', '--target', 'uploadfs']
    
    if port:
        upload_cmd.extend(['--upload-port', port])
    
    if ip:
        # OTA upload
        upload_cmd.extend(['--upload-protocol', 'espota'])
    
    print(f"{Colors.BLUE}Uploading SPIFFS image...{Colors.ENDC}")
    
    try:
        result = subprocess.run(upload_cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"{Colors.RED}Error uploading SPIFFS image:{Colors.ENDC}")
            print(result.stderr)
            return False
        
        print(f"{Colors.GREEN}SPIFFS image uploaded successfully!{Colors.ENDC}")
        return True
            
    except Exception as e:
        print(f"{Colors.RED}Error uploading SPIFFS: {str(e)}{Colors.ENDC}")
        return False

def main():
    args = parse_arguments()
    
    print(f"{Colors.HEADER}{Colors.BOLD}ESP32 SPIFFS File Uploader{Colors.ENDC}")
    print("=" * 50)
    
    data_dir = args.data
    
    # Clean data directory if requested
    if args.clean and os.path.exists(data_dir):
        print(f"{Colors.YELLOW}Cleaning data directory: {data_dir}{Colors.ENDC}")
        for item in os.listdir(data_dir):
            item_path = os.path.join(data_dir, item)
            if os.path.isfile(item_path):
                os.unlink(item_path)
            elif os.path.isdir(item_path):
                shutil.rmtree(item_path)
    
    # Check data directory
    has_files = check_data_directory(data_dir)
    
    if not has_files:
        print(f"{Colors.YELLOW}No files found to package. Add files to {data_dir}/ directory.{Colors.ENDC}")
        # Create a sample index.html file if directory is empty
        index_path = os.path.join(data_dir, 'index.html')
        if not os.path.exists(index_path):
            print(f"{Colors.BLUE}Creating sample index.html file...{Colors.ENDC}")
            with open(index_path, 'w') as f:
                f.write("""<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { background-color: #f2f2f2; font-family: Arial, sans-serif; margin: 0; padding: 20px; color: #333; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 5px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #0066cc; }
        .status { margin-top: 20px; padding: 15px; border-radius: 5px; background-color: #e6f3ff; }
        button { background-color: #0066cc; color: white; border: none; padding: 8px 15px; border-radius: 4px; cursor: pointer; }
        button:hover { background-color: #004080; }
    </style>
</head>
<body>
    <div class="container">
        <h1>0x3 ESP32 Web Server</h1>
        <p>This page is served from the ESP32's SPIFFS filesystem.</p>
        <div class="status">
            <p>Device status: <span id="status">Loading...</span></p>
            <p>Uptime: <span id="uptime">Loading...</span></p>
            <p>Free memory: <span id="memory">Loading...</span></p>
        </div>
        <p>
            <button onclick="fetchStatus()">Refresh Status</button>
            <button onclick="location.href='/update'">OTA Update</button>
        </p>
    </div>
    
    <script>
        function fetchStatus() {
            fetch('/api/v1/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('status').textContent = data.status;
                    document.getElementById('uptime').textContent = data.uptime;
                    document.getElementById('memory').textContent = data.freeHeap + ' bytes';
                })
                .catch(error => {
                    console.error('Error fetching status:', error);
                    document.getElementById('status').textContent = 'Error connecting to device';
                });
        }
        
        // Fetch on page load
        document.addEventListener('DOMContentLoaded', fetchStatus);
    </script>
</body>
</html>""")
    
    # Build SPIFFS binary
    spiffs_bin = build_spiffs_binary(data_dir)
    
    # Upload if requested and build was successful
    if args.upload and spiffs_bin:
        port = args.port
        if not port and not args.ip:
            port = find_serial_port()
            print(f"{Colors.BLUE}Auto-detected serial port: {port}{Colors.ENDC}")
        
        upload_spiffs(port, args.ip)
    
    print(f"\n{Colors.GREEN}SPIFFS operation complete.{Colors.ENDC}")

if __name__ == '__main__':
    main()
