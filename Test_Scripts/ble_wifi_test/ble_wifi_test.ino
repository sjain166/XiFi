/*
 * BLE Advertising with WiFi Background Test
 *
 * This script tests BLE advertising at 20ms intervals with two modes:
 * 1. WiFi Connected: WiFi maintains connection in background
 * 2. WiFi Disconnected: WiFi disabled
 *
 * Change WIFI_ENABLED below to switch modes (requires re-upload)
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h/Users/sidpro/Desktop/WorkPlace/UIUC/Fall_2025/CS 439/health-monitor/Test_Scripts/wifi_connect_test/wifi_connect_test.ino>
#include <WiFi.h>

// ============ CONFIGURATION ============
// Set to true for WiFi connected mode, false for WiFi disconnected mode
#define WIFI_ENABLED false

const char *ssid = "XiFi-Test-AP";
const char *password = "xifi2025";

// BLE Configuration
#define INTERVAL_20MS   32   // 20ms in BLE units (0.625ms each)
#define ADV_INTERVAL INTERVAL_20MS
#define PAYLOAD_SIZE 20
#define REPETITION_COUNT 2  // Send each packet multiple times

// ============ GLOBAL VARIABLES ============
BLEAdvertising *pAdvertising;

unsigned long sequenceNumber = 0;
unsigned long repetitionCounter = 0;
unsigned long totalBytesSent = 0;
unsigned long lastAdvTime = 0;
unsigned long advInterval = 20;  // Will be calculated in setup

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("BLE Advertising with WiFi Background Test");
  Serial.println("========================================");

  // ============ WiFi Setup ============
  #if WIFI_ENABLED
    Serial.println("\n[WiFi] Mode: ENABLED");
    Serial.println("[WiFi] Connecting to: " + String(ssid));

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[WiFi] Connected!");
      Serial.println("[WiFi] IP Address: " + WiFi.localIP().toString());
      Serial.println("[WiFi] Signal Strength (RSSI): " + String(WiFi.RSSI()) + " dBm");
    } else {
      Serial.println("\n[WiFi] Connection FAILED - continuing with BLE only");
    }
  #else
    Serial.println("\n[WiFi] Mode: DISABLED");
    WiFi.mode(WIFI_OFF);
  #endif

  // ============ BLE Setup ============
  Serial.println("\n[BLE] Initializing...");
  BLEDevice::init("ESP32-C5-Test");

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinInterval(ADV_INTERVAL);
  pAdvertising->setMaxInterval(ADV_INTERVAL);

  advInterval = (ADV_INTERVAL * 625) / 1000;  // Convert to ms

  Serial.println("[BLE] Configuration:");
  Serial.printf("  - Interval: %d units (%lu ms)\n", ADV_INTERVAL, advInterval);
  Serial.printf("  - Repetitions: %d\n", REPETITION_COUNT);
  Serial.printf("  - Effective rate: %.1f unique packets/sec\n",
                1000.0 / (advInterval * REPETITION_COUNT));
  Serial.printf("  - Payload size: %d bytes\n", PAYLOAD_SIZE);

  Serial.println("\n========================================");
  Serial.println("Starting BLE advertisements...");
  Serial.println("========================================\n");

  lastAdvTime = millis();
}

void loop() {
  unsigned long now = millis();

  // Send BLE advertisement at specified interval
  if (now - lastAdvTime >= advInterval) {
    lastAdvTime = now;
    sendAdvertisement();
  }

  // Check WiFi status periodically (every 30 seconds)
  #if WIFI_ENABLED
    static unsigned long lastWiFiCheck = 0;
    if (now - lastWiFiCheck >= 30000) {
      lastWiFiCheck = now;
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Still connected - RSSI: %d dBm\n", WiFi.RSSI());
      } else {
        Serial.println("[WiFi] WARNING: Connection lost!");
      }
    }
  #endif

  delay(1);
}

void sendAdvertisement() {
  // Prepare payload: 4 bytes sequence number + 16 bytes test string
  uint8_t payload[PAYLOAD_SIZE];
  memcpy(payload, &sequenceNumber, 4);
  const char* testString = "HELLO-C5-TEST!!!";
  memcpy(payload + 4, testString, 16);

  // Create advertisement data
  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x06);

  #if WIFI_ENABLED
    advertisementData.setName("C5-WiFi");
  #else
    advertisementData.setName("C5-NoWiFi");
  #endif

  String payloadStr((char*)payload, PAYLOAD_SIZE);
  advertisementData.setManufacturerData(payloadStr);

  // Send advertisement
  pAdvertising->stop();
  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->start();

  totalBytesSent += PAYLOAD_SIZE;
  repetitionCounter++;

  // Increment sequence number after sending REPETITION_COUNT times
  if (repetitionCounter >= REPETITION_COUNT) {
    sequenceNumber++;
    repetitionCounter = 0;

    // Print stats every 50 unique sequence numbers
    if (sequenceNumber % 50 == 0) {
      Serial.printf("[BLE] Seq: %lu (x%d reps), Total: %lu bytes, Uptime: %lu sec\n",
                    sequenceNumber, REPETITION_COUNT, totalBytesSent, millis()/1000);
    }
  }
}
