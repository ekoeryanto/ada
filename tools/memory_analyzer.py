#!/usr/bin/env python3
"""
ESP32 Memory Analyzer Tool
----------------------------
Analyzes memory usage in compiled firmware binaries for ESP32 devices.
Helps identify memory issues and optimize code.

Usage: 
    python memory_analyzer.py [options]

Options:
    --bin FILE       Path to firmware.bin file
    --elf FILE       Path to firmware.elf file
    --map FILE       Path to firmware.map file
    --analyze-stack  Analyze stack usage in functions
    --sections       Show memory usage by sections
    --symbols        Show symbols sorted by size
    --top N          Show top N largest symbols (default: 20)
    --help           Show this help message

Example:
    python memory_analyzer.py --elf .pio/build/d1_mini/firmware.elf --top 30
"""

import os
import sys
import re
import subprocess
import argparse
from pathlib import Path
import struct
import json

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
    parser = argparse.ArgumentParser(description='ESP32 Memory Analyzer')
    parser.add_argument('--bin', help='Path to firmware.bin file')
    parser.add_argument('--elf', help='Path to firmware.elf file')
    parser.add_argument('--map', help='Path to firmware.map file')
    parser.add_argument('--analyze-stack', action='store_true', help='Analyze stack usage')
    parser.add_argument('--sections', action='store_true', help='Show memory usage by sections')
    parser.add_argument('--symbols', action='store_true', help='Show symbols sorted by size')
    parser.add_argument('--top', type=int, default=20, help='Show top N largest symbols')
    
    return parser.parse_args()

def find_build_files(board='d1_mini'):
    """Find build files if not specified."""
    base_dir = Path('.pio/build/' + board)
    
    bin_file = base_dir / 'firmware.bin'
    elf_file = base_dir / 'firmware.elf'
    map_file = base_dir / 'firmware.map'
    
    return bin_file if bin_file.exists() else None, \
           elf_file if elf_file.exists() else None, \
           map_file if map_file.exists() else None

def analyze_binary_size(bin_file):
    """Analyze binary file size."""
    if not bin_file or not os.path.exists(bin_file):
        return None
    
    size = os.path.getsize(bin_file)
    return {
        'file': bin_file,
        'size': size,
        'formatted': format_size(size)
    }

def analyze_elf_file(elf_file):
    """Analyze ELF file using size utility."""
    if not elf_file or not os.path.exists(elf_file):
        return None
    
    result = subprocess.run(['xtensa-lx106-elf-size', '-A', elf_file], 
                            capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"Error running size: {result.stderr}")
        return None
    
    sections = {}
    lines = result.stdout.strip().split('\n')
    
    # Skip the header line
    for line in lines[1:]:
        parts = line.split()
        if len(parts) >= 3:
            name, size, *_ = parts
            sections[name] = int(size)
    
    return sections

def analyze_map_file(map_file, top_n=20):
    """Analyze map file for symbol sizes."""
    if not map_file or not os.path.exists(map_file):
        return None
    
    try:
        with open(map_file, 'r') as f:
            content = f.read()
            
        # Extract memory configuration
        mem_config = {}
        mem_config_pattern = r'([A-Za-z_]+)\s+:\s+org\s+=\s+0x([0-9a-fA-F]+),\s+len\s+=\s+0x([0-9a-fA-F]+)'
        for match in re.finditer(mem_config_pattern, content):
            name, org, length = match.groups()
            mem_config[name] = {
                'origin': int(org, 16),
                'length': int(length, 16)
            }
        
        # Extract symbols
        symbols = []
        symbol_pattern = r'^\s+(0x[0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+(.+?)(?:\s+(.+?))?$'
        
        for line in content.split('\n'):
            match = re.search(symbol_pattern, line)
            if match:
                addr, size_hex, name, section = match.groups() if len(match.groups()) == 4 else (*match.groups(), None)
                try:
                    size = int(size_hex, 16)
                    if size > 0 and not name.startswith('.'):
                        symbols.append({
                            'address': addr,
                            'size': size,
                            'name': name.strip(),
                            'section': section.strip() if section else ''
                        })
                except ValueError:
                    pass
        
        # Sort symbols by size (largest first)
        symbols.sort(key=lambda x: x['size'], reverse=True)
        
        return {
            'memory_config': mem_config,
            'symbols': symbols[:top_n]
        }
    
    except Exception as e:
        print(f"Error analyzing map file: {e}")
        return None

def analyze_stack_usage(elf_file):
    """Analyze stack usage of functions."""
    if not elf_file or not os.path.exists(elf_file):
        return None
    
    # This is a simplified version, full stack analysis requires more complex tools
    try:
        result = subprocess.run(['xtensa-lx106-elf-objdump', '-d', elf_file], 
                                capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Error running objdump: {result.stderr}")
            return None
        
        # Simple heuristic: look for stack adjustments
        functions = {}
        current_func = None
        stack_adj = 0
        
        for line in result.stdout.split('\n'):
            # Find function names
            func_match = re.search(r'^[0-9a-f]+ <([^>]+)>:', line)
            if func_match:
                if current_func and stack_adj > 0:
                    functions[current_func] = stack_adj
                current_func = func_match.group(1)
                stack_adj = 0
                continue
            
            # Look for stack pointer adjustments
            if current_func:
                # addi a1, a1, -X (stack grow)
                stack_match = re.search(r'addi\s+a1,\s*a1,\s*-(\d+)', line)
                if stack_match:
                    adj = int(stack_match.group(1))
                    stack_adj = max(stack_adj, adj)
        
        # Sort by stack usage (highest first)
        sorted_funcs = sorted(functions.items(), key=lambda x: x[1], reverse=True)
        return sorted_funcs[:20]  # Return top 20 functions
        
    except Exception as e:
        print(f"Error analyzing stack usage: {e}")
        return None

def format_size(size_bytes):
    """Format size in human-readable format."""
    if size_bytes < 1024:
        return f"{size_bytes} B"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes / 1024:.2f} KB"
    else:
        return f"{size_bytes / (1024 * 1024):.2f} MB"

def print_section_table(sections):
    """Print a table of memory sections."""
    print(f"\n{Colors.BOLD}Memory Usage by Section:{Colors.ENDC}")
    print(f"{Colors.BLUE}{'Section':<20}{'Size':<15}{'%':<10}{Colors.ENDC}")
    print("-" * 45)
    
    ram_sections = {}
    flash_sections = {}
    
    # Categorize sections
    flash_total = 0
    ram_total = 0
    
    for name, size in sections.items():
        if 'text' in name or 'rodata' in name:
            flash_sections[name] = size
            flash_total += size
        elif 'data' in name or 'bss' in name:
            ram_sections[name] = size
            ram_total += size
    
    # Print flash sections
    print(f"{Colors.YELLOW}FLASH SECTIONS:{Colors.ENDC}")
    for name, size in flash_sections.items():
        percentage = (size / flash_total) * 100 if flash_total else 0
        print(f"{name:<20}{format_size(size):<15}{percentage:>6.2f}%")
    print(f"{Colors.BOLD}{'Total Flash':<20}{format_size(flash_total):<15}{100:>6.2f}%{Colors.ENDC}")
    print()
    
    # Print RAM sections
    print(f"{Colors.YELLOW}RAM SECTIONS:{Colors.ENDC}")
    for name, size in ram_sections.items():
        percentage = (size / ram_total) * 100 if ram_total else 0
        print(f"{name:<20}{format_size(size):<15}{percentage:>6.2f}%")
    print(f"{Colors.BOLD}{'Total RAM':<20}{format_size(ram_total):<15}{100:>6.2f}%{Colors.ENDC}")

def print_symbols_table(symbols):
    """Print a table of symbols sorted by size."""
    if not symbols:
        return
    
    print(f"\n{Colors.BOLD}Largest Symbols:{Colors.ENDC}")
    print(f"{Colors.BLUE}{'Symbol':<50}{'Size':<15}{'Section':<20}{Colors.ENDC}")
    print("-" * 85)
    
    for symbol in symbols:
        name = symbol['name']
        if len(name) > 48:
            name = name[:45] + "..."
        print(f"{name:<50}{format_size(symbol['size']):<15}{symbol['section']:<20}")

def print_stack_usage(stack_data):
    """Print stack usage information."""
    if not stack_data:
        return
    
    print(f"\n{Colors.BOLD}Functions with Highest Stack Usage:{Colors.ENDC}")
    print(f"{Colors.BLUE}{'Function':<50}{'Stack Size':<15}{Colors.ENDC}")
    print("-" * 65)
    
    for func, stack in stack_data:
        name = func
        if len(name) > 48:
            name = name[:45] + "..."
        print(f"{name:<50}{stack:<15} bytes")

def main():
    args = parse_arguments()
    
    # Find build files if not specified
    if not (args.bin or args.elf or args.map):
        bin_file, elf_file, map_file = find_build_files()
        if not (bin_file or elf_file or map_file):
            print("Error: No build files found. Run build first or specify file paths.")
            sys.exit(1)
    else:
        bin_file = args.bin
        elf_file = args.elf
        map_file = args.map
    
    print(f"{Colors.HEADER}{Colors.BOLD}ESP32 Memory Analyzer{Colors.ENDC}")
    print("=" * 50)
    
    # Analyze binary size
    if bin_file:
        bin_info = analyze_binary_size(bin_file)
        if bin_info:
            print(f"\n{Colors.BOLD}Binary Size:{Colors.ENDC} {bin_info['formatted']}")
    
    # Analyze ELF sections
    if elf_file:
        sections = analyze_elf_file(elf_file)
        if sections and (args.sections or not (args.symbols or args.analyze_stack)):
            print_section_table(sections)
    
    # Analyze symbols from map file
    if map_file:
        map_info = analyze_map_file(map_file, args.top)
        if map_info and (args.symbols or not (args.sections or args.analyze_stack)):
            print_symbols_table(map_info['symbols'])
    
    # Analyze stack usage
    if elf_file and args.analyze_stack:
        stack_info = analyze_stack_usage(elf_file)
        if stack_info:
            print_stack_usage(stack_info)
    
    print(f"\n{Colors.GREEN}Analysis complete.{Colors.ENDC}")

if __name__ == '__main__':
    main()
