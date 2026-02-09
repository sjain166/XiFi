/*
 * HealthMonitor Sender - High-Rate IoT Sensor Node
 *
 * Simulates industrial sensor applications (e.g., vibration monitoring, health diagnostics)
 * that require frequent small updates with delay-sensitive data.
 *
 * Strategy:
 * - Baseline: Low-power BLE/UDP sends 20B packets every 20ms for real-time monitoring
 * - Adaptive: Buffers data and switches to bulk WiFi transmission when threshold reached
 * - Goal: Balance power efficiency with data freshness for time-sensitive analytics and response.
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// DEMO SIMULATION PARAMETERS
#define PHASE_DURATION_MS     30000  // 30 seconds per phase
#define WIFI_BACKGROUND_CONNECTED true

#define BUFFER_MAX_SIZE       10000
#define THRESHOLD_INITIAL     5000

// Adaptive threshold parameters
#define MIN_THRESHOLD         1000     // Minimum threshold (bytes)
#define MAX_THRESHOLD         9000  // Maximum threshold (bytes)
#define THRESHOLD_UPDATE_INTERVAL_MS 1000  // Update threshold every 2 seconds

#define INPUT_INTERVAL_MS     20
#define BASELINE_INTERVAL_MS  20
#define BASELINE_CHUNK_SIZE   16

// Dynamic parameters (changed by phase)
int currentTestMode = 2;  // Start with phase 1 (TEST_MODE 2)
int currentDelayTolerance = 5000;
int currentProb20 = 70;
int currentProb60 = 30;

#define UDP_BROADCAST_PORT    4210
#define BLE_ADV_INTERVAL_MS   32 // [32 = 20 ms | 80 = 50 ms | 160 = 100 ms]

#define MARKER_PIN_BASELINE   27
#define MARKER_PIN_BULK       4
#define MARKER_PIN_IDLE       5

#define DEMO_OUTPUT_INTERVAL_MS 50   // How often to output SENDER data
#define RATE_CALC_INTERVAL_MS   250 // How often to recalculate rates (2-3 seconds)

const char* ssid = "XiFi-Test-AP";
const char* password = "xifi2025";
const IPAddress broadcastIP(192, 168, 4, 255);

WiFiUDP udp;
BLEAdvertising* pAdvertising;

int bufferCounter = 0;
int currentThreshold = THRESHOLD_INITIAL;

unsigned long lastInputTime = 0;
unsigned long lastBaselineSendTime = 0;

unsigned long totalBytesSent = 0;
unsigned long baselineBytesSent = 0;
unsigned long bulkBytesSent = 0;

unsigned long sequenceNumber = 0;

unsigned long flushCount = 0;
unsigned long baselineSendCount = 0;

bool inFlushMode = false;


unsigned long lastDemoOutputTime = 0;
unsigned long totalInputBytes = 0;  
unsigned long lastInputBytesSnapshot = 0;
unsigned long lastOutputBytes = 0;
unsigned long lastRateCalcTime = 0;

float cachedInputRate = 0.0;
float cachedOutputRate = 0.0;
float cachedBufferFillRate = 0.0;

unsigned long lastThresholdUpdateTime = 0;

// Phase tracking
int currentPhase = 1;
unsigned long phaseStartTime = 0;

void outputDemoData(bool forceOutput = false);
void updateAdaptiveThreshold();
void checkAndSwitchPhase();

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(MARKER_PIN_BASELINE, OUTPUT);
  pinMode(MARKER_PIN_BULK, OUTPUT);
  pinMode(MARKER_PIN_IDLE, OUTPUT);

  setMarker(MARKER_PIN_IDLE, HIGH);

  // Serial.println("\n=== HealthMonitor Sender - DEMO MODE ===");
  // Serial.printf("WiFi Background: %s\n", WIFI_BACKGROUND_CONNECTED ? "ON" : "OFF");
  // Serial.printf("Buffer Max: %d, Threshold: %d\n", BUFFER_MAX_SIZE, currentThreshold);
  // Serial.println("Auto-switching phases every 30 seconds");
  // Serial.println("Phase 1: WiFi only | Phases 2-4: Hybrid BLE/WiFi\n");

  // Initialize both WiFi and BLE for demo
  connectWiFi();
  initBLE();

  lastInputTime = millis();
  lastBaselineSendTime = millis();
  phaseStartTime = millis();

  Serial.println("[PHASE 1] TEST_MODE=2, WiFi only for baseline+bulk");
}

void loop() {
  unsigned long now = millis();

  checkAndSwitchPhase();  // Check if we need to switch to next phase
  probabilisticInput();
  updateAdaptiveThreshold();
  checkAndSend();
  outputDemoData();

  delay(5);
}

void probabilisticInput() {
  unsigned long now = millis();

  if (now - lastInputTime < INPUT_INTERVAL_MS) return;
  lastInputTime = now;

  if (bufferCounter >= BUFFER_MAX_SIZE) return;

  int bytesToAdd = 0;
  int randVal = random(0, 100);

  if (randVal < currentProb20) {
    bytesToAdd = random(20, 25);
  } else {
    bytesToAdd = random(100, 120);
  }

  bufferCounter = min(bufferCounter + bytesToAdd, BUFFER_MAX_SIZE);
  totalInputBytes += bytesToAdd; 
}

void checkAndSend() {
  unsigned long now = millis();

  if (currentTestMode == 2) {
    // WiFi-only mode: Always flush immediately, no buffering
    // Send as fast as possible whenever there's data
    if (bufferCounter > 0) {
      if (!inFlushMode) {
        enterFlushMode();
      }
      flushBuffer();
    }
  } else {
    // Hybrid mode: Use threshold-based buffering
    if (bufferCounter >= currentThreshold && !inFlushMode) {
      enterFlushMode();
    }

    if (inFlushMode) {
      flushBuffer();
    } else {
      if (now - lastBaselineSendTime >= BASELINE_INTERVAL_MS) {
        sendBaseline();
      }
    }
  }
}

void sendBaseline() {
  if (bufferCounter < BASELINE_CHUNK_SIZE) return;

  lastBaselineSendTime = millis();

  setMarker(MARKER_PIN_BASELINE, HIGH);
  setMarker(MARKER_PIN_IDLE, LOW);

  if (currentTestMode == 1) {
    sendBLEBaseline();
  } else if (currentTestMode == 2) {
    sendUDPBaseline();
  }

  bufferCounter -= BASELINE_CHUNK_SIZE;
  baselineBytesSent += BASELINE_CHUNK_SIZE;
  totalBytesSent += BASELINE_CHUNK_SIZE;
  baselineSendCount++;

  setMarker(MARKER_PIN_BASELINE, LOW);
  setMarker(MARKER_PIN_IDLE, HIGH);
}

void sendBLEBaseline() {
  uint8_t payload[20];

  memcpy(payload, &sequenceNumber, 4);

  for (int i = 4; i < BASELINE_CHUNK_SIZE; i++) {
    payload[i] = random(0, 256);
  }

  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x06);
  advertisementData.setName("Health Monitor");

  String payloadStr((char*)payload, 20);
  advertisementData.setManufacturerData(payloadStr);

  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->start();
  delay(10);

  sequenceNumber++;
}

void sendUDPBaseline() {
  uint8_t payload[BASELINE_CHUNK_SIZE + 4];

  memcpy(payload, &sequenceNumber, 4);

  for (int i = 4; i < BASELINE_CHUNK_SIZE + 4; i++) {
    payload[i] = random(0, 256);
  }

  udp.beginPacket(broadcastIP, UDP_BROADCAST_PORT);
  udp.write(payload, BASELINE_CHUNK_SIZE + 4);
  udp.endPacket();

  sequenceNumber++;
}

void enterFlushMode() {
  inFlushMode = true;
  flushCount++;

  setMarker(MARKER_PIN_IDLE, LOW);
  setMarker(MARKER_PIN_BASELINE, LOW);
  setMarker(MARKER_PIN_BULK, HIGH);

  if (currentTestMode == 1) {
    pAdvertising->stop();

    if (!WIFI_BACKGROUND_CONNECTED) {
      connectWiFi();
    }
  }

  // outputDemoData(true);
}

void flushBuffer() {
    while(bufferCounter > 0) { 
      int chunkSize = min(bufferCounter, 1400);

      uint8_t payload[1400 + 8];
      memcpy(payload, &sequenceNumber, 4);
      memcpy(payload + 4, &totalBytesSent, 4);

      for (int i = 8; i < chunkSize + 8; i++) {
        payload[i] = random(0, 256);
      }

      udp.beginPacket(broadcastIP, UDP_BROADCAST_PORT);
      udp.write(payload, chunkSize + 8);
      udp.endPacket();

      bufferCounter -= chunkSize;
      bulkBytesSent += chunkSize;
      totalBytesSent += chunkSize;
      sequenceNumber++;

      outputDemoData(true);

      delay(50);
    }

    exitFlushMode();
  }

void exitFlushMode() {
  inFlushMode = false;

  if (currentTestMode == 1) {
    pAdvertising->start();

    if (!WIFI_BACKGROUND_CONNECTED) {
      WiFi.disconnect(true);
      delay(100);
    }
  }

  setMarker(MARKER_PIN_BULK, LOW);
  setMarker(MARKER_PIN_IDLE, HIGH);

  //outputDemoData(true);
}

void updateAdaptiveThreshold() {
  unsigned long now = millis();
  if (now - lastThresholdUpdateTime < random(250, 1000)) return;

  lastThresholdUpdateTime = now;
  if (cachedBufferFillRate > 0) {
    float newThreshold = cachedBufferFillRate * (currentDelayTolerance / 1000.0);
    currentThreshold = constrain((int)newThreshold, MIN_THRESHOLD, MAX_THRESHOLD);
  } else if (cachedBufferFillRate < 0) {
    currentThreshold = MIN_THRESHOLD;
  }
}

void initBLE() {
  Serial.println("[BLE] Initializing...");
  BLEDevice::init("ESP32-C5");
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  // 20 ms Interval for BLE Advertising
  pAdvertising->setMinInterval(BLE_ADV_INTERVAL_MS);
  pAdvertising->setMaxInterval(BLE_ADV_INTERVAL_MS);
  Serial.println("[BLE] Ready (100ms interval)");
}

void connectWiFi() {
  Serial.print("[WiFi] Connecting");
  WiFi.begin(ssid, password);

  int timeout = 40;
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] Connection failed!");
  }
}

void outputDemoData(bool forceOutput) {
  unsigned long now = millis();

  if (!forceOutput && (now - lastDemoOutputTime < DEMO_OUTPUT_INTERVAL_MS)) return;
  if (now - lastRateCalcTime >= RATE_CALC_INTERVAL_MS || lastRateCalcTime == 0) {
    unsigned long timeDelta = now - lastRateCalcTime;
    if (timeDelta == 0) timeDelta = 1;
    cachedInputRate = ((totalInputBytes - lastInputBytesSnapshot) * 1000.0) / timeDelta;
    cachedOutputRate = ((totalBytesSent - lastOutputBytes) * 1000.0) / timeDelta;
    cachedBufferFillRate = cachedInputRate - cachedOutputRate;
    lastInputBytesSnapshot = totalInputBytes;
    lastOutputBytes = totalBytesSent;
    lastRateCalcTime = now;
  }

  lastDemoOutputTime = now;
  int mode = inFlushMode ? 1 : 0;

  if (currentTestMode == 2){
    mode = 1;
  }

  // Format: SENDER,timestamp,buffer,threshold,mode,input_rate,output_rate,buffer_fill_rate,phase,test_mode,delay_tolerance,prob_20,prob_60
  Serial.print("SENDER,");
  Serial.print(now);
  Serial.print(",");
  Serial.print(bufferCounter);
  Serial.print(",");
  Serial.print(currentThreshold);
  Serial.print(",");
  Serial.print(mode);
  Serial.print(",");
  Serial.print(cachedInputRate, 2);
  Serial.print(",");
  Serial.print(cachedOutputRate, 2);
  Serial.print(",");
  Serial.print(cachedBufferFillRate, 2);
  Serial.print(",");
  Serial.print(currentPhase);
  Serial.print(",");
  Serial.print(currentTestMode);
  Serial.print(",");
  Serial.print(currentDelayTolerance);
  Serial.print(",");
  Serial.print(currentProb20);
  Serial.print(",");
  Serial.println(currentProb60);
}

void setMarker(int pin, int state) {
  digitalWrite(pin, state);
}

void checkAndSwitchPhase() {
  unsigned long now = millis();
  unsigned long phaseElapsed = now - phaseStartTime;

  // Check if 30 seconds have passed
  if (phaseElapsed < PHASE_DURATION_MS) return;

  // Switch to next phase
  currentPhase++;
  if (currentPhase > 4) currentPhase = 1;  // Loop back to phase 1

  phaseStartTime = now;

  // Clear buffer and reset counters
  bufferCounter = 0;
  totalBytesSent = 0;
  baselineBytesSent = 0;
  bulkBytesSent = 0;
  totalInputBytes = 0;
  lastInputBytesSnapshot = 0;
  lastOutputBytes = 0;
  sequenceNumber = 0;
  flushCount = 0;
  baselineSendCount = 0;
  inFlushMode = false;
  cachedInputRate = 0.0;
  cachedOutputRate = 0.0;
  cachedBufferFillRate = 0.0;

  // Configure phase parameters
  switch (currentPhase) {
    case 1:
      // Phase 1: TEST_MODE 2 (WiFi only)
      currentTestMode = 2;
      currentDelayTolerance = 5000;  // Not used in mode 2, but set for consistency
      currentProb20 = 70;
      currentProb60 = 30;
      Serial.println("\n[PHASE 1] TEST_MODE=2, WiFi only for baseline+bulk");
      Serial.println("Demonstrating pure WiFi transmission for comparison");
      // Stop BLE if running
      if (pAdvertising) {
        pAdvertising->stop();
      }
      break;

    case 2:
      // Phase 2: TEST_MODE 1, DELAY_TOLERANCE 5000ms
      currentTestMode = 1;
      currentDelayTolerance = 5000;
      currentProb20 = 70;
      currentProb60 = 30;
      Serial.println("\n[PHASE 2] TEST_MODE=1, DELAY_TOLERANCE=5000ms");
      Serial.println("Hybrid BLE baseline + WiFi bulk, moderate delay tolerance");
      break;

    case 3:
      // Phase 3: TEST_MODE 1, DELAY_TOLERANCE 12000ms
      currentTestMode = 1;
      currentDelayTolerance = 12000;
      currentProb20 = 70;
      currentProb60 = 30;
      Serial.println("\n[PHASE 3] TEST_MODE=1, DELAY_TOLERANCE=12000ms");
      Serial.println("Hybrid mode with HIGH delay tolerance (more BLE, less WiFi)");
      break;

    case 4:
      // Phase 4: TEST_MODE 1, DELAY_TOLERANCE 2000ms
      currentTestMode = 1;
      currentDelayTolerance = 2000;
      currentProb20 = 70;
      currentProb60 = 30;
      Serial.println("\n[PHASE 4] TEST_MODE=1, DELAY_TOLERANCE=2000ms");
      Serial.println("Hybrid mode with LOW delay tolerance (less BLE, more WiFi)");
      break;
  }
}
