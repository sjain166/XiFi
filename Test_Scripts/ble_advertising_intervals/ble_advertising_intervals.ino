#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>


BLEAdvertising *pAdvertising;

#define INTERVAL_100MS  160   
#define INTERVAL_50MS   80    
#define INTERVAL_20MS   32   
#define INTERVAL_10MS   16

#define ADV_INTERVAL INTERVAL_20MS

#define PAYLOAD_SIZE 20
#define REPETITION_COUNT 2  // Send each packet 3 times (change to 1 for no repetition, 2 for 2x)

unsigned long sequenceNumber = 0;
unsigned long repetitionCounter = 0;  // Track how many times current seq has been sent
unsigned long totalBytesSent = 0;
unsigned long lastAdvTime = 0;
unsigned long advInterval = 100;

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32-C5");
  Serial.println("BLE initialized");
  
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);

  pAdvertising->setMinInterval(ADV_INTERVAL);
  pAdvertising->setMaxInterval(ADV_INTERVAL);

  advInterval = (ADV_INTERVAL * 625) / 1000;
  Serial.printf("Interval: %d units (%lu ms)\n", ADV_INTERVAL, advInterval);
  Serial.printf("Effective rate: %.1f unique packets/sec\n", 1000.0 / (advInterval * REPETITION_COUNT));
  Serial.println("\nStarting advertisements with repetitive sending...\n");

  lastAdvTime = millis();
}

void loop() {
  unsigned long now = millis();

  if (now - lastAdvTime >= advInterval) {
    lastAdvTime = now;
    sendAdvertisement();
  }

  delay(1);
}

void sendAdvertisement() {
  uint8_t payload[PAYLOAD_SIZE];
  memcpy(payload, &sequenceNumber, 4);
  const char* testString = "HELLO-C5-TEST!!!";
  memcpy(payload + 4, testString, 16);

  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x06);
  advertisementData.setName("C5");

  String payloadStr((char*)payload, PAYLOAD_SIZE);
  advertisementData.setManufacturerData(payloadStr);

  pAdvertising->stop();
  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->start();

  totalBytesSent += PAYLOAD_SIZE;
  repetitionCounter++;

  // Only increment sequence number after sending REPETITION_COUNT times
  if (repetitionCounter >= REPETITION_COUNT) {
    sequenceNumber++;
    repetitionCounter = 0;

    // Print every 50 unique sequence numbers
    if (sequenceNumber % 50 == 0) {
      Serial.printf("Unique Seq: %lu (sent %dx each), Total: %lu bytes\n",
                    sequenceNumber, REPETITION_COUNT, totalBytesSent);
    }
  }
}
