#include <bluefruit.h>

#define STATUS_INTERVAL_MS 5000
#define DEDUP_BUFFER_SIZE 100 

volatile unsigned long totalPacketsSeen = 0;
volatile unsigned long totalBytesReceived = 0;
volatile unsigned long lastSequenceNumber = 0;
volatile unsigned long firstSequenceNumber = 0;
volatile unsigned long packetsReceived = 0;
volatile unsigned long packetsLost = 0;
volatile unsigned long duplicatesReceived = 0; 
volatile bool firstPacketReceived = false;

unsigned long lastStatusTime = 0;
unsigned long testStartTime = 0;

uint32_t seenSequences[DEDUP_BUFFER_SIZE];
uint16_t seenSeqIndex = 0;

bool isSequenceSeen(uint32_t seqNum) {
  for (int i = 0; i < DEDUP_BUFFER_SIZE; i++) {
    if (seenSequences[i] == seqNum) {
      return true;
    }
  }
  return false;
}

void markSequenceSeen(uint32_t seqNum) {
  seenSequences[seenSeqIndex] = seqNum;
  seenSeqIndex = (seenSeqIndex + 1) % DEDUP_BUFFER_SIZE;
}

void scan_callback(ble_gap_evt_adv_report_t* report) {
  totalPacketsSeen++;

  uint8_t nameBuffer[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
  uint8_t mfgBuffer[BLE_GAP_ADV_SET_DATA_SIZE_MAX];

  memset(nameBuffer, 0, sizeof(nameBuffer));
  memset(mfgBuffer, 0, sizeof(mfgBuffer));

  bool isC5Device = false;
  if (Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, nameBuffer, sizeof(nameBuffer))) {
    if (strcmp((char*)nameBuffer, "C5") == 0) {
      isC5Device = true;
    }
  }

  if (!isC5Device) {
    if (Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, nameBuffer, sizeof(nameBuffer))) {
      if (strcmp((char*)nameBuffer, "C5") == 0) {
        isC5Device = true;
      }
    }
  }

  if (!isC5Device) {
    Bluefruit.Scanner.resume();
    return;
  }

  uint16_t mfgLen = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, mfgBuffer, sizeof(mfgBuffer));

  if (mfgLen >= 4) {
    uint32_t seqNum;
    memcpy(&seqNum, mfgBuffer, 4); 

    if (isSequenceSeen(seqNum)) {
      duplicatesReceived++;
      Bluefruit.Scanner.resume();
      return;
    }

    markSequenceSeen(seqNum);

    if (!firstPacketReceived) {
      firstPacketReceived = true;
      firstSequenceNumber = seqNum;
      lastSequenceNumber = seqNum;
      packetsReceived++;
      Serial.printf("[C5] First unique packet! SEQ=%lu RSSI=%d\n", seqNum, report->rssi);
    } else {
      if (seqNum != lastSequenceNumber + 1) {
        unsigned long lost = seqNum - lastSequenceNumber - 1;
        packetsLost += lost;
        Serial.printf("[LOSS] Gap detected! Expected %lu, got %lu. Lost %lu unique packets\n",
                      lastSequenceNumber + 1, seqNum, lost);
      }

      lastSequenceNumber = seqNum;
      packetsReceived++;

      if (packetsReceived % 50 == 0) {
        unsigned long packetsExpected = (lastSequenceNumber - firstSequenceNumber) + 1;
        float lossRate = (packetsExpected > 0) ? (packetsLost * 100.0) / packetsExpected : 0.0;
        Serial.printf("[C5] SEQ=%lu | Unique RX=%lu | Lost=%lu | Dups=%lu | Loss=%.2f%% | RSSI=%d\n",
                      seqNum, packetsReceived, packetsLost, duplicatesReceived, lossRate, report->rssi);
      }
    }
  }

  Bluefruit.Scanner.resume();
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n=== nRF52840 BLE Debug Receiver ===");
  Serial.println("With deduplication for repetitive sending\n");

  for (int i = 0; i < DEDUP_BUFFER_SIZE; i++) {
    seenSequences[i] = 0xFFFFFFFF; 
  }

  Bluefruit.begin();
  Bluefruit.setName("nRF52840-Debug");

  const uint16_t interval = 32;
  const uint16_t window   = 32;

  Bluefruit.Scanner.setInterval(interval, window);
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.setRxCallback(scan_callback);

  Serial.println("[BLE] Starting continuous scan (100% duty cycle)...");
  Serial.println("[READY] Listening for ALL advertisements...\n");

  Bluefruit.Scanner.start(0);

  testStartTime = millis();
  lastStatusTime = millis();
}

void loop() {
  printStatus();
  delay(10);
}

void printStatus() {
  unsigned long now = millis();

  if (now - lastStatusTime < STATUS_INTERVAL_MS) return;
  lastStatusTime = now;

  if (!firstPacketReceived) {
    Serial.println("\n[Waiting for first packet...]");
    return;
  }

  float elapsedSec = (now - testStartTime) / 1000.0;

  unsigned long packetsExpected = (lastSequenceNumber - firstSequenceNumber) + 1;
  float lossRate = 0.0;
  if (packetsExpected > 0) {
    lossRate = (packetsLost * 100.0) / packetsExpected;
  }

  float receiveRate = packetsReceived / elapsedSec;
  unsigned long totalPacketsReceived = packetsReceived + duplicatesReceived;
  float avgRepetition = (packetsReceived > 0) ? (float)totalPacketsReceived / packetsReceived : 0.0;

  Serial.println("\n========== STATISTICS (Cumulative) ==========");
  Serial.printf("Time: %.1fs\n", elapsedSec);
  Serial.printf("Sequence Range: %lu to %lu\n", firstSequenceNumber, lastSequenceNumber);
  Serial.printf("Unique Packets Expected: %lu\n", packetsExpected);
  Serial.printf("Unique Packets Received: %lu\n", packetsReceived);
  Serial.printf("Duplicate Packets: %lu\n", duplicatesReceived);
  Serial.printf("Total Packets Received: %lu (unique + dups)\n", totalPacketsReceived);
  Serial.printf("Avg Repetition Received: %.2fx\n", avgRepetition);
  Serial.printf("Unique Packets Lost: %lu\n", packetsLost);
  Serial.printf("Loss Rate: %.2f%% (based on unique packets)\n", lossRate);
  Serial.printf("Unique Receive Rate: %.1f packets/sec\n", receiveRate);
  Serial.println("============================================\n");
}
