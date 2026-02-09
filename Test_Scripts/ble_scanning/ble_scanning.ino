/*
 * BLE Scanning Test
 *
 * This script continuously scans for BLE devices at 1000ms intervals.
 * Duration: 15 seconds
 *
 * Hardware: ESP32-C5
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ============ CONFIGURATION ============
#define SCAN_INTERVAL_MS 1000  // 1000ms scan interval
#define TEST_DURATION_SEC 15
#define INITIAL_DELAY_SEC 5

BLEScan* pBLEScan;
int totalDevicesFound = 0;
int totalScans = 0;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Count devices but don't print to avoid cluttering output
    totalDevicesFound++;
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("BLE Scanning Test");
  Serial.println("========================================");
  Serial.printf("Scan Interval: %d ms\n", SCAN_INTERVAL_MS);
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

  // Initialize BLE
  Serial.println("[BLE] Initializing...");
  BLEDevice::init("ESP32-C5-Scanner");

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // Active scan uses more power but gets more data
  pBLEScan->setInterval(100);     // Scan interval in BLE units (0.625ms each)
  pBLEScan->setWindow(99);        // Scan window in BLE units

  Serial.println("[BLE] Initialized successfully");
  Serial.println("\n========================================");
  Serial.println("Starting BLE scanning");
  Serial.println("========================================\n");
}

void loop() {
  static unsigned long testStartTime = millis();
  static unsigned long lastScanTime = 0;
  unsigned long now = millis();

  // Perform scan at specified interval
  if (now - lastScanTime >= SCAN_INTERVAL_MS) {
    lastScanTime = now;
    totalScans++;

    unsigned long elapsed = (now - testStartTime) / 1000;

    // Perform scan (1 second duration)
    BLEScanResults* foundDevices = pBLEScan->start(1, false);
    int devicesThisScan = foundDevices->getCount();

    Serial.printf("[%lu sec] Scan #%d - Found %d devices\n",
                  elapsed, totalScans, devicesThisScan);

    pBLEScan->clearResults();  // Clear results to free memory
  }

  // End test after duration
  if (now - testStartTime >= (TEST_DURATION_SEC * 1000)) {
    Serial.println("\n========================================");
    Serial.println("TEST COMPLETE");
    Serial.println("========================================");
    Serial.printf("Duration: %d seconds\n", TEST_DURATION_SEC);
    Serial.printf("Total Scans: %d\n", totalScans);
    Serial.printf("Total Devices Found: %d\n", totalDevicesFound);
    if (totalScans > 0) {
      Serial.printf("Avg Devices/Scan: %.1f\n", (float)totalDevicesFound / totalScans);
    }
    Serial.println("========================================\n");
    Serial.println("Entering idle mode - you can stop measurements");

    // Stop scanning and stay in idle
    pBLEScan->stop();
    while (true) {
      delay(1000);
    }
  }

  delay(10);  // Small delay
}
