#include <unity.h>
#include <Arduino.h>
#include "system_manager.h"

// Test setup function
void setUp(void) {
  // Set up test dependencies
}

// Test teardown function
void tearDown(void) {
  // Clean up after tests
}

// Test system manager initialization
void test_system_manager_init(void) {
  SystemManager sysManager;
  TEST_ASSERT_TRUE(sysManager.init());
}

// Test system reporting functions
void test_system_info_reporting(void) {
  SystemManager sysManager;
  sysManager.init();
  
  // Check that we can get system information
  TEST_ASSERT_NOT_NULL(sysManager.getChipModel());
  TEST_ASSERT_TRUE(sysManager.getFreeMem() > 0);
  TEST_ASSERT_TRUE(sysManager.getUptime() >= 0);
}

// Test status LED functionality
void test_system_status_led(void) {
  SystemManager sysManager;
  sysManager.init();
  
  // Test LED status changes
  sysManager.setStatusLED(true);  // LED on
  TEST_ASSERT_TRUE(sysManager.getLEDStatus());
  
  sysManager.setStatusLED(false); // LED off
  TEST_ASSERT_FALSE(sysManager.getLEDStatus());
}

void setup() {
  // Wait for serial connection
  delay(2000);
  
  // Start test framework
  UNITY_BEGIN();
  
  // Run tests
  RUN_TEST(test_system_manager_init);
  RUN_TEST(test_system_info_reporting);
  RUN_TEST(test_system_status_led);
  
  // Complete test run
  UNITY_END();
}

void loop() {
  // Nothing to do here
}

// For native testing (not on ESP32 hardware)
#ifndef ARDUINO
int main(int argc, char **argv) {
  setup();
  while (1) {
    loop();
  }
  return 0;
}
#endif
