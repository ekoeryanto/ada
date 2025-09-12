#!/usr/bin/env python3
"""
ESP32 Serial Monitor Plus
--------------------------
Enhanced serial monitor for ESP32 with JSON parsing, filtering, and data export.

Usage:
    python serial_monitor_plus.py [options]

Options:
    --port PORT      Serial port (auto-detects if not specified)
    --baud RATE      Baud rate (default: 115200)
    --filter TEXT    Only show lines containing this text
    --json           Parse and colorize JSON output
    --log FILE       Save output to log file
    --export FILE    Export parsed JSON data to CSV
    --help           Show this help message

Example:
    python serial_monitor_plus.py --port /dev/ttyUSB0 --json --log output.txt
"""

import os
import sys
import re
import argparse
import serial
import serial.tools.list_ports
import json
import csv
import platform
import time
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
    parser = argparse.ArgumentParser(description='ESP32 Serial Monitor Plus')
    parser.add_argument('--port', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--filter', help='Only show lines containing this text')
    parser.add_argument('--json', action='store_true', help='Parse and colorize JSON output')
    parser.add_argument('--log', help='Save output to log file')
    parser.add_argument('--export', help='Export parsed JSON data to CSV')
    
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

def prettify_json(json_str):
    """Parse and prettify JSON string."""
    try:
        data = json.loads(json_str)
        return json.dumps(data, indent=2)
    except:
        return None

def colorize_json(json_str):
    """Add color to a prettified JSON string."""
    lines = json_str.split('\n')
    colored_lines = []
    
    for line in lines:
        # Colorize keys
        line = re.sub(r'(".*?"): ', f"{Colors.BLUE}\\1{Colors.ENDC}: ", line)
        
        # Colorize string values
        line = re.sub(r': (".*?")(,)?$', f": {Colors.GREEN}\\1{Colors.ENDC}\\2", line)
        
        # Colorize numbers
        line = re.sub(r': (-?\d+\.?\d*)(,)?$', f": {Colors.YELLOW}\\1{Colors.ENDC}\\2", line)
        
        # Colorize booleans
        line = re.sub(r': (true|false)(,)?$', f": {Colors.RED}\\1{Colors.ENDC}\\2", line)
        
        # Colorize null
        line = re.sub(r': (null)(,)?$', f": {Colors.RED}\\1{Colors.ENDC}\\2", line)
        
        colored_lines.append(line)
    
    return '\n'.join(colored_lines)

def extract_json_data_for_csv(lines):
    """Extract consistent JSON data for CSV export."""
    json_objects = []
    
    for line in lines:
        try:
            # Find JSON data in the line
            json_match = re.search(r'{.*}', line)
            if json_match:
                json_str = json_match.group(0)
                data = json.loads(json_str)
                json_objects.append(data)
        except:
            continue
    
    if not json_objects:
        return None, None
    
    # Find common keys across all objects
    keys = set()
    for obj in json_objects:
        keys.update(obj.keys())
    
    # Prepare data for CSV
    csv_data = []
    for obj in json_objects:
        row = {}
        for key in keys:
            row[key] = obj.get(key, '')
        csv_data.append(row)
    
    return list(keys), csv_data

def write_csv(filename, headers, data):
    """Write data to CSV file."""
    with open(filename, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=headers)
        writer.writeheader()
        writer.writerows(data)

def main():
    args = parse_arguments()
    
    # Set up serial port
    port = args.port
    if not port:
        port = find_serial_port()
        print(f"{Colors.BLUE}Auto-detected serial port: {port}{Colors.ENDC}")
    
    baud_rate = args.baud
    
    print(f"{Colors.HEADER}{Colors.BOLD}ESP32 Serial Monitor Plus{Colors.ENDC}")
    print("=" * 50)
    print(f"Port: {port}")
    print(f"Baud rate: {baud_rate}")
    if args.filter:
        print(f"Filter: '{args.filter}'")
    if args.log:
        print(f"Logging to: {args.log}")
    print("Press Ctrl+C to exit")
    print("-" * 50)
    
    # Initialize log file if needed
    log_file = None
    if args.log:
        try:
            log_file = open(args.log, 'w')
            log_file.write(f"--- ESP32 Serial Monitor Log ---\n")
            log_file.write(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            log_file.write(f"Port: {port}\n")
            log_file.write(f"Baud rate: {baud_rate}\n")
            log_file.write("-" * 50 + "\n")
        except Exception as e:
            print(f"{Colors.RED}Error opening log file: {str(e)}{Colors.ENDC}")
            log_file = None
    
    # Track received lines for JSON export
    all_lines = []
    
    # Open serial port
    try:
        with serial.Serial(port, baud_rate, timeout=0.1) as ser:
            buffer = ""
            
            while True:
                try:
                    data = ser.read(256)
                    if data:
                        text = data.decode('utf-8', errors='replace')
                        buffer += text
                        
                        # Process complete lines
                        lines = buffer.split('\n')
                        buffer = lines.pop()  # Keep the incomplete line
                        
                        for line in lines:
                            line = line.strip()
                            if not line:
                                continue
                            
                            # Apply filter if specified
                            if args.filter and args.filter not in line:
                                continue
                            
                            timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                            
                            # Store for potential export
                            all_lines.append(line)
                            
                            # Look for JSON content
                            if args.json and (line.startswith('{') and line.endswith('}')) or '{' in line and '}' in line:
                                try:
                                    # For lines that are only JSON
                                    if line.startswith('{') and line.endswith('}'):
                                        pretty_json = prettify_json(line)
                                        if pretty_json:
                                            colored_json = colorize_json(pretty_json)
                                            print(f"{Colors.CYAN}[{timestamp}]{Colors.ENDC}\n{colored_json}")
                                            if log_file:
                                                log_file.write(f"[{timestamp}]\n{pretty_json}\n")
                                            continue
                                    
                                    # For lines with embedded JSON
                                    json_match = re.search(r'{.*}', line)
                                    if json_match:
                                        json_part = json_match.group(0)
                                        pretty_json = prettify_json(json_part)
                                        if pretty_json:
                                            prefix = line[:json_match.start()]
                                            suffix = line[json_match.end():]
                                            colored_json = colorize_json(pretty_json)
                                            print(f"{Colors.CYAN}[{timestamp}]{Colors.ENDC} {prefix}")
                                            print(f"{colored_json}")
                                            if suffix:
                                                print(suffix)
                                            if log_file:
                                                log_file.write(f"[{timestamp}] {prefix}\n{pretty_json}\n{suffix}\n")
                                            continue
                                except Exception as e:
                                    # If JSON processing fails, fall back to normal output
                                    pass
                            
                            # Normal output
                            print(f"{Colors.CYAN}[{timestamp}]{Colors.ENDC} {line}")
                            if log_file:
                                log_file.write(f"[{timestamp}] {line}\n")
                                log_file.flush()
                                
                except KeyboardInterrupt:
                    break
                except serial.SerialException as e:
                    print(f"{Colors.RED}Serial error: {str(e)}{Colors.ENDC}")
                    break
                except Exception as e:
                    print(f"{Colors.RED}Error: {str(e)}{Colors.ENDC}")
                    time.sleep(1)
    
    except serial.SerialException as e:
        print(f"{Colors.RED}Failed to open serial port {port}: {str(e)}{Colors.ENDC}")
        sys.exit(1)
    
    finally:
        if log_file:
            log_file.close()
            print(f"{Colors.GREEN}Log saved to {args.log}{Colors.ENDC}")
        
        # Export JSON data to CSV if requested
        if args.export and all_lines:
            print(f"{Colors.BLUE}Exporting data to CSV...{Colors.ENDC}")
            headers, data = extract_json_data_for_csv(all_lines)
            
            if headers and data:
                try:
                    write_csv(args.export, headers, data)
                    print(f"{Colors.GREEN}CSV data exported to {args.export}{Colors.ENDC}")
                except Exception as e:
                    print(f"{Colors.RED}Error exporting CSV: {str(e)}{Colors.ENDC}")
            else:
                print(f"{Colors.YELLOW}No consistent JSON data found for CSV export{Colors.ENDC}")

if __name__ == '__main__':
    main()
