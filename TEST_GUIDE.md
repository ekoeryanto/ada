# ESP32 Project Testing Guide

This document provides an overview of the testing strategy and setup for the ESP32 Universal Template project.

## Testing Strategy

The project follows a multi-layered testing approach:

1. **Unit Testing** - Testing individual components in isolation
2. **Integration Testing** - Testing components working together
3. **Static Analysis** - Code quality checks
4. **Continuous Integration** - Automated testing on each code change

## Test Environment Setup

### Local Development Testing

For local testing, the following environments are configured:

1. **Native Tests** - Run on your development machine, no hardware required
   - Fast execution
   - Suitable for logic and algorithm testing
   - Uses mocks for hardware-specific functionality

2. **ESP32 Hardware Tests** - Run on actual ESP32 devices
   - Tests hardware interactions
   - Validates real-world behavior
   - Requires physical ESP32 hardware

### Required Tools

- PlatformIO (includes Unity test framework)
- cppcheck (static analysis)
- cpplint (style checking)

## Running Tests

### Basic Test Commands

```bash
# Run all tests
platformio test

# Run only native tests (no hardware required)
platformio test -e test_desktop

# Run only hardware tests
platformio test -e test

# Run full test suite with reports
./tools/run_tests.sh
```

### Test Output

Test results are saved in the `test_results/` directory, including:
- Native test logs
- ESP32 test build logs
- Static analysis reports

## Writing New Tests

### Test File Structure

Create a new test file in an appropriate subdirectory:

```cpp
#include <unity.h>
#include <Arduino.h>
#include "component_to_test.h"
#include "test_utils.h"  // For shared test utilities

void setUp(void) {
    // Setup code runs before each test
}

void tearDown(void) {
    // Teardown code runs after each test
}

void test_function_name(void) {
    // Your test assertions here
    TEST_ASSERT_EQUAL(expected, actual);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_function_name);
    UNITY_END();
}

void loop() {
    // Nothing needed here for tests
}

// For native tests
#ifndef ARDUINO
int main(int argc, char **argv) {
    setup();
    return 0;
}
#endif
```

### Mocking Hardware Dependencies

Use the utilities in `test_utils.h` to mock hardware dependencies when writing native tests.

## Continuous Integration

The GitHub Actions workflow in `.github/workflows/ci.yml` performs:

1. Building the project for all supported boards
2. Running native tests
3. Building hardware tests (but not running, as CI has no physical hardware)
4. Running static analysis

CI runs automatically on:
- Pushes to main, master, and develop branches
- Pull requests to these branches
- Manual triggers via GitHub Actions UI

## Troubleshooting Tests

Common issues and solutions:

1. **Test build fails**
   - Check that all required libraries are properly included
   - Verify that mocks are properly implemented for native tests

2. **Tests pass locally but fail in CI**
   - Timing issues: CI may run tests faster/slower than local machine
   - Environment differences: Check for hardcoded paths or environment-specific code

3. **Static analysis failures**
   - Code style issues: Follow the style guide and fix linting errors
   - Potential bugs: Address warnings from cppcheck
