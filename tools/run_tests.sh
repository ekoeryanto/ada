#!/bin/bash

# ESP32 Test Runner Script
# Runs all tests and generates reports

# Terminal colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}====================================${NC}"
echo -e "${YELLOW}    ESP32 Test Runner Script        ${NC}"
echo -e "${YELLOW}====================================${NC}"

# Create test results directory
RESULTS_DIR="test_results"
mkdir -p $RESULTS_DIR

# Run native tests
echo -e "\n${YELLOW}Running native tests...${NC}"
platformio test -e test_desktop | tee $RESULTS_DIR/native_tests.log
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Native tests passed!${NC}"
else
    echo -e "${RED}Native tests failed!${NC}"
    TESTS_FAILED=1
fi

# Build tests for ESP32
echo -e "\n${YELLOW}Building ESP32 tests...${NC}"
platformio test -e test --without-uploading | tee $RESULTS_DIR/esp32_build.log
if [ $? -eq 0 ]; then
    echo -e "${GREEN}ESP32 test build successful!${NC}"
else
    echo -e "${RED}ESP32 test build failed!${NC}"
    TESTS_FAILED=1
fi

# Code quality checks
echo -e "\n${YELLOW}Running code quality checks...${NC}"

# Check if cppcheck is installed
if command -v cppcheck &> /dev/null; then
    echo "Running cppcheck..."
    cppcheck --enable=all --inline-suppr --suppress=missingIncludeSystem src/ include/ > $RESULTS_DIR/cppcheck.log 2>&1
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}cppcheck passed!${NC}"
    else
        echo -e "${RED}cppcheck found issues. See $RESULTS_DIR/cppcheck.log${NC}"
        TESTS_FAILED=1
    fi
else
    echo -e "${YELLOW}cppcheck not installed, skipping...${NC}"
fi

# Check if cpplint is installed
if command -v cpplint &> /dev/null; then
    echo "Running cpplint..."
    cpplint --filter=-legal/copyright,-whitespace/line_length,-readability/casting,-build/include_subdir src/*.cpp include/*.h > $RESULTS_DIR/cpplint.log 2>&1
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}cpplint passed!${NC}"
    else
        echo -e "${RED}cpplint found issues. See $RESULTS_DIR/cpplint.log${NC}"
        TESTS_FAILED=1
    fi
else
    echo -e "${YELLOW}cpplint not installed, skipping...${NC}"
fi

# Output summary
echo -e "\n${YELLOW}====================================${NC}"
echo -e "${YELLOW}          Test Summary              ${NC}"
echo -e "${YELLOW}====================================${NC}"

if [ "$TESTS_FAILED" == "1" ]; then
    echo -e "${RED}Some tests failed. Check logs in $RESULTS_DIR${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi
