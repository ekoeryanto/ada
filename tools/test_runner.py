#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess

class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def run_tests(args):
    """Run tests based on arguments"""
    print(f"{Colors.HEADER}Running ESP32 Tests{Colors.ENDC}")
    
    # Determine which test environment to use
    if args.native:
        env = "test_desktop"
        print(f"{Colors.BLUE}Using native test environment (no hardware required){Colors.ENDC}")
    elif args.hardware:
        env = "test"
        print(f"{Colors.BLUE}Using hardware test environment{Colors.ENDC}")
    else:
        # Default: run both
        env = None
        print(f"{Colors.BLUE}Running all test environments{Colors.ENDC}")
    
    # Build command
    cmd = ["platformio", "test"]
    if env:
        cmd.extend(["-e", env])
    
    if args.verbose:
        cmd.append("-v")
    
    if args.filter:
        cmd.extend(["-f", args.filter])
    
    # Generate reports if requested
    if args.report:
        # Create reports directory if it doesn't exist
        if not os.path.exists("test_results"):
            os.makedirs("test_results")
        print(f"{Colors.BLUE}Generating test reports in test_results/{Colors.ENDC}")
        
        # If we're running the script directly
        if not args.no_script:
            try:
                print(f"{Colors.BLUE}Running test script with reports...{Colors.ENDC}")
                subprocess.run(["./tools/run_tests.sh"], check=True)
            except subprocess.CalledProcessError as e:
                print(f"{Colors.RED}Error running test script: {e}{Colors.ENDC}")
                return
            except FileNotFoundError:
                print(f"{Colors.RED}Error: run_tests.sh script not found{Colors.ENDC}")
                return
        else:
            # If we're just using platformio directly with reports
            report_file = os.path.join("test_results", "test_output.log")
            with open(report_file, "w") as f:
                subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)
            print(f"{Colors.GREEN}Test report written to {report_file}{Colors.ENDC}")
            return
    
    # Run the command
    if not args.report or args.no_script:
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError as e:
            print(f"{Colors.RED}Tests failed with error code {e.returncode}{Colors.ENDC}")

def setup_test_parser(subparsers):
    """Set up the argument parser for the test command"""
    parser = subparsers.add_parser('test', help='Run unit and integration tests')
    parser.add_argument('--native', action='store_true', help='Run only native tests (no hardware required)')
    parser.add_argument('--hardware', action='store_true', help='Run only hardware tests')
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose output')
    parser.add_argument('-f', '--filter', help='Filter tests by name')
    parser.add_argument('--report', action='store_true', help='Generate test reports')
    parser.add_argument('--no-script', action='store_true', help='Do not use the run_tests.sh script')
    return parser
