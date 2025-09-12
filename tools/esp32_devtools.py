#!/usr/bin/env python3
"""
0x3 ESP32 Development Tools
----------------------------
A collection of development tools for ESP32 projects.

Usage:
    python esp32_devtools.py [command] [options]

Commands:
    memory       - Analyze memory usage in firmware
    spiffs       - Build and upload SPIFFS filesystem
    config       - Generate configuration files
    monitor      - Enhanced serial monitoring with JSON support
    wifi         - Scan WiFi networks
    test         - Run unit and integration tests
    help         - Show this help message

Example:
    python esp32_devtools.py memory --elf .pio/build/esp32doit-devkit-v1/firmware.elf
    python esp32_devtools.py spiffs --data data --upload
    python esp32_devtools.py monitor --port /dev/ttyUSB0 --json

Run 'python esp32_devtools.py [command] --help' for command-specific help.
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path
from test_runner import run_tests, setup_test_parser

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
    parser = argparse.ArgumentParser(
        description='ESP32 Development Tools',
        usage=__doc__
    )
    parser.add_argument('command', choices=['memory', 'spiffs', 'config', 'monitor', 'wifi', 'test', 'help'])
    
    # Parse just the first argument to determine the command
    args = parser.parse_args(sys.argv[1:2])
    
    return args

def print_help():
    """Print full help text."""
    print(__doc__)

def run_tool(tool_name, args):
    """Run the specified tool with arguments."""
    tools_dir = Path(__file__).parent
    tool_script = tools_dir / f"{tool_name}.py"
    
    if not tool_script.exists():
        print(f"{Colors.RED}Error: Tool script not found: {tool_script}{Colors.ENDC}")
        return 1
    
    cmd = [sys.executable, str(tool_script)] + args
    return subprocess.call(cmd)

def main():
    if len(sys.argv) < 2:
        print_help()
        return 0
    
    args = parse_arguments()
    
    if args.command == 'help':
        print_help()
        return 0
    
    # Handle test command separately
    if args.command == 'test':
        # Create a new parser for test-specific arguments
        parser = argparse.ArgumentParser(description='Run ESP32 tests')
        subparsers = parser.add_subparsers(dest='ignored')  # Just to match the structure
        test_parser = setup_test_parser(subparsers)
        test_args = test_parser.parse_args(sys.argv[2:])
        return run_tests(test_args)
    
    # Map commands to tools
    tool_map = {
        'memory': 'memory_analyzer',
        'spiffs': 'spiffs_uploader',
        'config': 'config_generator',
        'monitor': 'serial_monitor_plus',
        'wifi': 'wifi_scanner'
    }
    
    tool_name = tool_map.get(args.command)
    if not tool_name:
        print(f"{Colors.RED}Error: Unknown command: {args.command}{Colors.ENDC}")
        print_help()
        return 1
    
    # Pass the remaining arguments to the tool
    return run_tool(tool_name, sys.argv[2:])

if __name__ == '__main__':
    sys.exit(main())
