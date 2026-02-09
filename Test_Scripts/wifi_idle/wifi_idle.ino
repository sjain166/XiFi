/*
 * WiFi Idle Test
 *
 * This script connects to WiFi and stays idle (maintaining connection only).
 * No active data transmission or operations.
 * Duration: 15 seconds
 *
 * Hardware: ESP32-C5
 */

#include <WiFi.h>

// ============ CONFIGURATION ============
const char *ssid = "XiFi-Test-AP";
const char *password = "xifi2025";

#define TEST_DURATION_SEC 15
#define INITIAL_DELAY_SEC 5

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("WiFi Idle Test");
  Serial.println("========================================");
  Serial.printf("SSID: %s\n", ssid);
  Serial.printf("Test Duration: %d seconds\n", TEST_DURATION_SEC);
  Serial.println("========================================");

  // Initial delay with countdown
  Serial.printf("\nStarting in %d seconds...\n", INITIAL_DELAY_SEC);
  Serial.println("Please start your power measurement now!");

  for (int i = INITIAL_DELAY_SEC; i > 0; i--) {
    Serial.printf("  %d...\n", i);
    delay(1000);
  }

  Serial.println("\n>>> TEST STARTING NOW <<<\n");

  // Connect to WiFi
  Serial.printf("[WiFi] Connecting to %s...\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startConnect = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startConnect) < 10000) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected!");
    Serial.printf("  IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("  Channel: %d\n", WiFi.channel());
    Serial.printf("  MAC: %s\n", WiFi.macAddress().c_str());
  } else {
    Serial.println("\n[WiFi] Connection FAILED!");
    Serial.println("Test aborted.");
    while (true) delay(1000);
  }

  Serial.println("\n========================================");
  Serial.println("WiFi connected - entering IDLE mode");
  Serial.printf("Will remain idle for %d seconds\n", TEST_DURATION_SEC);
  Serial.println("========================================\n");
}

void loop() {
  static unsigned long testStartTime = millis();
  static unsigned long lastStatusPrint = 0;
  unsigned long now = millis();

  // Print status every 5 seconds
  if (now - lastStatusPrint >= 5000) {
    lastStatusPrint = now;
    unsigned long elapsed = (now - testStartTime) / 1000;

    Serial.printf("[%lu sec] WiFi Idle - RSSI: %d dBm, Status: %s\n",
                  elapsed,
                  WiFi.RSSI(),
                  WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  }

  // End test after duration
  if (now - testStartTime >= (TEST_DURATION_SEC * 1000)) {
    Serial.println("\n========================================");
    Serial.println("TEST COMPLETE");
    Serial.println("========================================");
    Serial.printf("Duration: %d seconds\n", TEST_DURATION_SEC);
    Serial.printf("Final RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.println("========================================\n");
    Serial.println("Entering idle mode - you can stop measurements");

    // Stay in idle
    while (true) {
      delay(1000);
    }
  }

  delay(100);  // Small delay to prevent tight loop
}
