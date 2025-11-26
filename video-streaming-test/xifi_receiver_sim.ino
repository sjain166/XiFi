#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

// ==========================
// Config simulation params
// ==========================
#define BUFFER_MAX          500       // simulated video buffer size
#define BUFFER_THRESHOLD    200       // when below, advertise NEED_DATA
#define VIDEO_CONSUME_RATE  5         // bytes removed per loop
#define LOOP_DELAY          50        // ms per loop
#define WIFI_BURST_SIZE     120       // simulated chunk of incoming data
#define WIFI_BURST_MIN_MS   300       // minimum delay between bursts
#define WIFI_BURST_MAX_MS   2500      // maximum delay between bursts

const char* BLE_DEVICE_NAME = "ESP-NEED-DATA-Receiver";

// ==========================
// global state variables
// ==========================
int videoBuffer = BUFFER_MAX;
bool isAdvertising = false;

unsigned long nextBurstTime = 0;
BLEAdvertising* pAdvertising;

// ==========================
// BLE control functions
// ==========================
void startBleNeedData() {
  if (isAdvertising) return;

  Serial.println("[BLE] START advertising NEED_DATA");

  BLEAdvertisementData advData;
  advData.setManufacturerData("NEED_DATA");

  pAdvertising->setAdvertisementData(advData);
  pAdvertising->start();

  isAdvertising = true;
}

void stopBleNeedData() {
  if (!isAdvertising) return;

  Serial.println("[BLE] STOP advertising");
  pAdvertising->stop();
  isAdvertising = false;
}

// ==========================
// Simulate Wi-Fi bursts
// ==========================
void simulateWifiBurst() {
  Serial.printf("[WiFi] Incoming burst: +%d bytes\n", WIFI_BURST_SIZE);

  stopBleNeedData();

  videoBuffer += WIFI_BURST_SIZE;
  if (videoBuffer > BUFFER_MAX) videoBuffer = BUFFER_MAX;

  // Schedule next burst randomly
  nextBurstTime = millis() + random(WIFI_BURST_MIN_MS, WIFI_BURST_MAX_MS);
}

// ==========================
// Arduino setup & loop
// ==========================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== XiFi: Video Receiver Simulation ===");

  randomSeed(analogRead(0)); // seed randomness from ADC noise

  BLEDevice::init(BLE_DEVICE_NAME);

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0);

  Serial.println("[BLE] Ready");
  Serial.println("[System] Starting simulation...\n");

  nextBurstTime = millis() + random(WIFI_BURST_MIN_MS, WIFI_BURST_MAX_MS); // schedule first burst randomly
}

void loop() {

  // simulate constant playback consumption
  videoBuffer -= VIDEO_CONSUME_RATE;
  if (videoBuffer < 0) videoBuffer = 0;

  // logging
  Serial.printf("Buffer=%4d  (%s)\n",
      videoBuffer,
      isAdvertising ? "ADV_ON" : "ADV_OFF"
  );

  // trigger BLE NEED_DATA Advertisement when buffer below threshold
  if (videoBuffer < BUFFER_THRESHOLD && !isAdvertising) {
    startBleNeedData();
  }

  // randomized next WiFi burst timing
  unsigned long now = millis();
  if (now >= nextBurstTime) {
    simulateWifiBurst();
  }

  delay(LOOP_DELAY);
}
