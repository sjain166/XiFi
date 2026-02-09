/*
 * WiFi Connect/Disconnect Test
 *
 * This script performs 10 WiFi connection/disconnection cycles:
 * - 5 second initial delay for starting power measurements
 * - Measures connection time for each cycle
 * - Calculates and displays average connection time
 * - No delay between disconnect and reconnect
 *
 * Hardware: ESP32-C5
 */

#include <WiFi.h>

// ============ CONFIGURATION ============
const char *ssid = "XiFi-Test-AP";
const char *password = "xifi2025";

#define NUM_CYCLES 10
#define INITIAL_DELAY_SEC 5
#define CONNECTION_TIMEOUT_MS 10000  // 10 second timeout per attempt

// ============ GLOBAL VARIABLES ============
unsigned long connectionTimes[NUM_CYCLES];
int successfulConnections = 0;
int failedConnections = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("WiFi Connect/Disconnect Test");
  Serial.println("========================================");
  Serial.printf("SSID: %s\n", ssid);
  Serial.printf("Number of cycles: %d\n", NUM_CYCLES);
  Serial.println("========================================");

  // Initial delay with countdown
  Serial.printf("\nStarting in %d seconds...\n", INITIAL_DELAY_SEC);
  Serial.println("Please start your power measurement now!");

  for (int i = INITIAL_DELAY_SEC; i > 0; i--) {
    Serial.printf("  %d...\n", i);
    delay(1000);
  }

  Serial.println("\n>>> TEST STARTING NOW <<<\n");
  Serial.println("========================================");
}

void loop() {
  // Run all cycles once
  for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
    Serial.println();
    Serial.printf("========== CYCLE %d/%d ==========\n", cycle + 1, NUM_CYCLES);

    // Connect to WiFi and measure time
    unsigned long connectTime = connectToWiFi(cycle + 1);
    connectionTimes[cycle] = connectTime;

    if (connectTime > 0) {
      successfulConnections++;
      Serial.printf("[✓] Connected in %lu ms\n", connectTime);

      // Display connection info
      Serial.printf("    IP Address: %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("    RSSI: %d dBm\n", WiFi.RSSI());
      Serial.printf("    Channel: %d\n", WiFi.channel());

      // Brief connected state (100ms)
      delay(100);

      // Disconnect
      Serial.println("[→] Disconnecting...");
      WiFi.disconnect(true, false);  // Disconnect but don't erase credentials
      Serial.println("[✓] Disconnected");

    } else {
      failedConnections++;
      Serial.println("[✗] Connection FAILED (timeout)");
      WiFi.disconnect(true, false);
    }

    // No delay - immediately proceed to next cycle
  }

  // Print final results
  printResults();

  Serial.println("\n========================================");
  Serial.println("TEST COMPLETE - Entering idle mode");
  Serial.println("========================================\n");

  // Stay in idle mode
  while (true) {
    delay(1000);
  }
}

unsigned long connectToWiFi(int cycleNum) {
  Serial.printf("[→] Connecting to %s...\n", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  unsigned long currentTime;

  // Wait for connection with timeout
  while (WiFi.status() != WL_CONNECTED) {
    currentTime = millis();

    if (currentTime - startTime > CONNECTION_TIMEOUT_MS) {
      return 0;  // Timeout - return 0 to indicate failure
    }

    delay(50);
  }

  unsigned long connectionTime = millis() - startTime;
  return connectionTime;
}

void printResults() {
  Serial.println("\n");
  Serial.println("========================================");
  Serial.println("           FINAL RESULTS");
  Serial.println("========================================");
  Serial.printf("Total Cycles:        %d\n", NUM_CYCLES);
  Serial.printf("Successful:          %d\n", successfulConnections);
  Serial.printf("Failed:              %d\n", failedConnections);
  Serial.println("========================================");

  // Print individual connection times
  Serial.println("\nConnection Times:");
  Serial.println("Cycle  |  Time (ms)  |  Status");
  Serial.println("-------|-------------|----------");

  unsigned long totalTime = 0;
  int validConnections = 0;

  for (int i = 0; i < NUM_CYCLES; i++) {
    Serial.printf("  %2d   |   %6lu    |  ", i + 1, connectionTimes[i]);

    if (connectionTimes[i] > 0) {
      Serial.println("Success");
      totalTime += connectionTimes[i];
      validConnections++;
    } else {
      Serial.println("Failed");
    }
  }

  Serial.println("========================================");

  // Calculate statistics
  if (validConnections > 0) {
    unsigned long avgTime = totalTime / validConnections;

    // Find min and max
    unsigned long minTime = CONNECTION_TIMEOUT_MS;
    unsigned long maxTime = 0;

    for (int i = 0; i < NUM_CYCLES; i++) {
      if (connectionTimes[i] > 0) {
        if (connectionTimes[i] < minTime) minTime = connectionTimes[i];
        if (connectionTimes[i] > maxTime) maxTime = connectionTimes[i];
      }
    }

    Serial.println("\nStatistics (successful connections only):");
    Serial.printf("  Average:   %lu ms\n", avgTime);
    Serial.printf("  Minimum:   %lu ms\n", minTime);
    Serial.printf("  Maximum:   %lu ms\n", maxTime);
    Serial.printf("  Range:     %lu ms\n", maxTime - minTime);
    Serial.printf("  Success Rate: %.1f%%\n", (validConnections * 100.0) / NUM_CYCLES);
  } else {
    Serial.println("\nNo successful connections!");
  }

  Serial.println("========================================");
}
