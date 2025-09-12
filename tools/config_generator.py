#!/usr/bin/env python3
"""
ESP32 Configuration Generator
------------------------------
Tool to generate configuration files for the ESP32 project.

Usage:
    python config_generator.py [options]

Options:
    --output FILE    Output file (default: include/config.h)
    --input FILE     Input template file (default: include/config_template.h)
    --env FILE       Environment file for values (default: .env)
    --interactive    Run in interactive mode
    --project NAME   Project name
    --version VER    Project version
    --author AUTH    Project author
    --help           Show this help message

Example:
    python config_generator.py --interactive
    python config_generator.py --project "MyProject" --version "1.0.0" --author "Me"
"""

import os
import sys
import re
import argparse
import json
import uuid
import secrets
import string
import socket
import getpass
from datetime import datetime
from pathlib import Path

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
    parser = argparse.ArgumentParser(description='ESP32 Configuration Generator')
    parser.add_argument('--output', default='include/config.h', help='Output file')
    parser.add_argument('--input', default='include/config_template.h', help='Input template file')
    parser.add_argument('--env', default='.env', help='Environment file for values')
    parser.add_argument('--interactive', action='store_true', help='Run in interactive mode')
    parser.add_argument('--project', help='Project name')
    parser.add_argument('--version', help='Project version')
    parser.add_argument('--author', help='Project author')
    
    return parser.parse_args()

def read_env_file(env_file):
    """Read environment variables from .env file."""
    env_vars = {}
    
    if os.path.exists(env_file):
        with open(env_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    key, value = line.split('=', 1)
                    env_vars[key.strip()] = value.strip().strip('"\'')
    
    return env_vars

def generate_hostname(project_name):
    """Generate a safe hostname from project name."""
    if not project_name:
        return '0x3-esp32'
    
    # Convert to lowercase, replace spaces with hyphens
    hostname = project_name.lower().replace(' ', '-')
    # Remove any non-alphanumeric characters except hyphens
    hostname = re.sub(r'[^a-z0-9\-]', '', hostname)
    # Ensure it starts with a letter
    if not hostname or not hostname[0].isalpha():
        hostname = 'esp-' + hostname
    # Truncate to 63 characters (DNS limit)
    hostname = hostname[:63]
    # Remove trailing hyphens
    hostname = hostname.rstrip('-')
    
    return hostname

def generate_secure_password(length=16):
    """Generate a secure random password."""
    alphabet = string.ascii_letters + string.digits
    password = ''.join(secrets.choice(alphabet) for i in range(length))
    return password

def prompt_with_default(prompt, default=None):
    """Prompt for input with a default value."""
    if default:
        user_input = input(f"{prompt} [{default}]: ").strip()
        return user_input if user_input else default
    else:
        return input(f"{prompt}: ").strip()

def process_template(template_file, values):
    """Process template file and replace placeholders."""
    if not os.path.exists(template_file):
        print(f"{Colors.RED}Error: Template file not found: {template_file}{Colors.ENDC}")
        return None
    
    with open(template_file, 'r') as f:
        template = f.read()
    
    # Add current date to values
    values['GENERATED_DATE'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Replace placeholders
    for key, value in values.items():
        placeholder = f"{{${key}$}}"
        template = template.replace(placeholder, str(value))
    
    # Check for any remaining placeholders
    remaining_placeholders = re.findall(r'{\$[A-Z_]+\$}', template)
    if remaining_placeholders:
        print(f"{Colors.YELLOW}Warning: Some placeholders were not replaced:{Colors.ENDC}")
        for placeholder in set(remaining_placeholders):
            print(f"  - {placeholder}")
    
    return template

def interactive_mode():
    """Run in interactive mode to gather configuration values."""
    print(f"{Colors.HEADER}{Colors.BOLD}ESP32 Configuration Generator - Interactive Mode{Colors.ENDC}")
    print("=" * 60)
    print("Please enter values for your configuration:")
    
    values = {}
    
    # Project Information
    print(f"\n{Colors.BLUE}Project Information:{Colors.ENDC}")
    values['PROJECT_NAME'] = prompt_with_default("Project Name", "0x3-ESP32")
    values['PROJECT_VERSION'] = prompt_with_default("Project Version", "1.0.0")
    values['PROJECT_AUTHOR'] = prompt_with_default("Project Author", getpass.getuser())
    
    # Network Configuration
    print(f"\n{Colors.BLUE}Network Configuration:{Colors.ENDC}")
    values['HOSTNAME'] = prompt_with_default(
        "Hostname (lowercase, no spaces)", 
        generate_hostname(values['PROJECT_NAME'])
    )
    
    # Generate random device ID
    device_id = f"{uuid.uuid4().hex[:8]}"
    values['DEVICE_ID'] = prompt_with_default("Device ID", device_id)
    
    # Security Configuration
    print(f"\n{Colors.BLUE}Security Configuration:{Colors.ENDC}")
    use_secure = prompt_with_default("Generate secure passwords? (y/n)", "y").lower()
    
    if use_secure.startswith('y'):
        ap_password = generate_secure_password(12)
        ota_password = generate_secure_password(16)
        print(f"{Colors.GREEN}Generated secure passwords{Colors.ENDC}")
    else:
        ap_password = "0x3Config"
        ota_password = "0x3Update"
    
    values['AP_PASSWORD'] = prompt_with_default("AP Password", ap_password)
    values['OTA_USERNAME'] = prompt_with_default("OTA Username", "admin")
    values['OTA_PASSWORD'] = prompt_with_default("OTA Password", ota_password)
    
    # Advanced Features
    print(f"\n{Colors.BLUE}Advanced Features:{Colors.ENDC}")
    values['ENABLE_MDNS'] = "true" if prompt_with_default("Enable mDNS? (y/n)", "y").lower().startswith('y') else "false"
    values['ENABLE_TELEMETRY'] = "true" if prompt_with_default("Enable telemetry? (y/n)", "y").lower().startswith('y') else "false"
    values['ENABLE_JSON_LOGGING'] = "true" if prompt_with_default("Enable JSON logging? (y/n)", "y").lower().startswith('y') else "false"
    
    return values

def main():
    args = parse_arguments()
    
    # Initialize configuration values
    config_values = {}
    
    # Read from environment file if it exists
    env_vars = read_env_file(args.env)
    config_values.update(env_vars)
    
    # Interactive mode takes precedence
    if args.interactive:
        user_values = interactive_mode()
        config_values.update(user_values)
    else:
        # Use command line arguments if provided
        if args.project:
            config_values['PROJECT_NAME'] = args.project
        if args.version:
            config_values['PROJECT_VERSION'] = args.version
        if args.author:
            config_values['PROJECT_AUTHOR'] = args.author
        
        # Fill in defaults for required values
        if 'PROJECT_NAME' not in config_values:
            config_values['PROJECT_NAME'] = "0x3-ESP32"
        if 'PROJECT_VERSION' not in config_values:
            config_values['PROJECT_VERSION'] = "1.0.0"
        if 'PROJECT_AUTHOR' not in config_values:
            config_values['PROJECT_AUTHOR'] = getpass.getuser()
        if 'HOSTNAME' not in config_values:
            config_values['HOSTNAME'] = generate_hostname(config_values['PROJECT_NAME'])
        if 'AP_PASSWORD' not in config_values:
            config_values['AP_PASSWORD'] = "0x3Config"
        if 'OTA_USERNAME' not in config_values:
            config_values['OTA_USERNAME'] = "admin"
        if 'OTA_PASSWORD' not in config_values:
            config_values['OTA_PASSWORD'] = "0x3Update"
        if 'DEVICE_ID' not in config_values:
            config_values['DEVICE_ID'] = f"{uuid.uuid4().hex[:8]}"
    
    # Process template
    output_content = process_template(args.input, config_values)
    if not output_content:
        sys.exit(1)
    
    # Write output file
    output_dir = os.path.dirname(args.output)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    try:
        with open(args.output, 'w') as f:
            f.write(output_content)
        
        print(f"{Colors.GREEN}Configuration file generated: {args.output}{Colors.ENDC}")
        
        # Also write .env file for future use
        if args.interactive:
            with open(args.env, 'w') as f:
                for key, value in config_values.items():
                    f.write(f"{key}=\"{value}\"\n")
            
            print(f"{Colors.GREEN}Environment file updated: {args.env}{Colors.ENDC}")
        
    except Exception as e:
        print(f"{Colors.RED}Error writing configuration file: {str(e)}{Colors.ENDC}")
        sys.exit(1)

if __name__ == '__main__':
    main()
