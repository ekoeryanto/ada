#include <unity.h>
#include <Arduino.h>
#include "web_server.h"
#include "system_manager.h"
#include "test_utils.h"

// Integration test for web server and system manager
// This test ensures the web server can properly report system status

SystemManager* sysManager;
WebServer* webServer;

// Test setup function
void setUp(void) {
  sysManager = new SystemManager();
  sysManager->init();
  
  webServer = new WebServer();
  webServer->init(sysManager);
}

// Test teardown function
void tearDown(void) {
  delete webServer;
  delete sysManager;
  TestHelpers::resetTestState();
}

// Test web server initialization
void test_web_server_init(void) {
  WebServer server;
  TEST_ASSERT_TRUE(server.init(sysManager));
}

// Test system info API endpoint 
void test_api_system_info(void) {
  // In a real test, we would make an HTTP request to the API endpoint
  // and verify the response. For this mock test, we'll just check that
  // the JSON generation for system info works correctly.
  String json = webServer->generateSystemInfoJSON();
  
  // Check that the JSON contains key system information
  TEST_ASSERT_TRUE(json.indexOf("\"chipModel\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"freeMem\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"uptime\"") >= 0);
  TEST_ASSERT_TRUE(json.indexOf("\"wifiStatus\"") >= 0);
}

void setup() {
  // Wait for serial connection
  delay(2000);
  
  // Start test framework
  UNITY_BEGIN();
  
  // Run tests
  RUN_TEST(test_web_server_init);
  RUN_TEST(test_api_system_info);
  
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
