

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SEND_INTERVAL_MS  20   // Options: 20, 50, 100

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
uint32_t sequenceNumber = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("[BLE] Client connected!");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("[BLE] Client disconnected!");
      BLEDevice::startAdvertising();
      Serial.println("[BLE] Advertising restarted");
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== BLE Connected Sender ===");
  Serial.printf("Send interval: %d ms\n", SEND_INTERVAL_MS);
  Serial.printf("Payload size: 20 bytes\n\n");

  BLEDevice::init("ESP32-Sender");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); 
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] Server started, waiting for client...");
}

void loop() {
  static unsigned long lastSendTime = 0;
  static bool connectionLogged = false;
  unsigned long now = millis();

  if (deviceConnected && !connectionLogged) {
    Serial.println("[BLE] Starting to send notifications...");
    connectionLogged = true;
  }
  if (!deviceConnected && connectionLogged) {
    connectionLogged = false;
  }

  if (deviceConnected && (now - lastSendTime >= SEND_INTERVAL_MS)) {
    lastSendTime = now;

    uint8_t payload[20];

    memcpy(payload, &sequenceNumber, 4);

    memcpy(payload + 4, &now, 4);

    for (int i = 8; i < 20; i++) {
      payload[i] = (i - 8) & 0xFF;
    }

    pCharacteristic->setValue(payload, 20);
    pCharacteristic->notify();

    if (sequenceNumber < 5 || sequenceNumber % 50 == 0) {
      Serial.printf("[DEBUG] notify() called for seq: %u, clients subscribed: %d\n",
                    sequenceNumber, pServer->getConnectedCount());
    }

    sequenceNumber++;
  }

  delay(1);
}
