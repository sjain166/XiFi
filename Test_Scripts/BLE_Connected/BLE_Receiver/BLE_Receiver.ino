
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Must match sender UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice* myDevice;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteService* pRemoteService = nullptr;  
static BLEClient* pClient = nullptr; 

uint32_t lastSeq = 0;
uint32_t packetsReceived = 0;
uint32_t packetsLost = 0;

void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                    uint8_t* pData, size_t length, bool isNotify) {
  // if (length >= 4) {
  //   // Extract sequence number
  //   uint32_t seq;
  //   memcpy(&seq, pData, 4);

  //   // Check for packet loss
  //   if (packetsReceived > 0) {
  //     uint32_t expected = lastSeq + 1;
  //     if (seq != expected) {
  //       uint32_t lost = seq - expected;
  //       packetsLost += lost;
  //       Serial.printf("Expected %u, got %u. Lost %u packets\n", expected, seq, lost);
  //     }
  //   }

  //   lastSeq = seq;
  //   packetsReceived++;

  //   // Print every 50 packets
  //   if (seq % 50 == 0) {
  //     float lossRate = (packetsReceived + packetsLost) > 0 ?
  //                      (packetsLost * 100.0) / (packetsReceived + packetsLost) : 0.0;
  //     Serial.printf("Received seq: %u | Total: %u | Lost: %u (%.2f%%)\n",
  //                   seq, packetsReceived, packetsLost, lossRate);
  //   }
  // }

  Serial.println("========== NOTIFICATION RECEIVED ==========");
  Serial.printf("Length: %d, isNotify: %d\n", length, isNotify);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("[BLE] Connected to server");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("[BLE] Disconnected from server");
  }
};

bool connectToServer() {
  Serial.print("[BLE] Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  Serial.println("[BLE] Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to server
  pClient->connect(myDevice);
  Serial.println("[BLE] Connected to server");

  pClient->setMTU(517);
  Serial.println("[BLE] MTU set to 517");

  // Get service (use global to keep it alive!)
  Serial.println("[BLE] Getting service...");
  pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("[ERROR] Failed to find service");
    pClient->disconnect();
    return false;
  }
  Serial.println("[BLE] Service found");

  // Get characteristic
  Serial.println("[BLE] Getting characteristic...");
  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("[ERROR] Failed to find characteristic");
    pClient->disconnect();
    return false;
  }
  Serial.println("[BLE] Characteristic found");

  Serial.println("[TEST] Attempting to read characteristic value...");
  if (pRemoteCharacteristic->canRead()) {
    String  value = pRemoteCharacteristic->readValue();
    Serial.printf("[TEST] Successfully read %d bytes from characteristic\n", value.length());
    if (value.length() >= 4) {
      uint32_t testSeq;
      memcpy(&testSeq, value.c_str(), 4);
      Serial.printf("[TEST] Read sequence number: %u\n", testSeq);
    }
  } else {
    Serial.println("[TEST] Characteristic does not support READ!");
  }

  if (pRemoteCharacteristic->canNotify()) {
    Serial.println("[BLE] Characteristic supports notifications");
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("[BLE] Registered for notifications");
  } else {
    Serial.println("[ERROR] Characteristic does NOT support notifications!");
  }

  connected = true;
  Serial.println("[BLE] Connection complete - ready to receive");
  return true;
}

// Scan callback
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Check if this is our sender
    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      Serial.println("[BLE] Found sender device!");
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== BLE Connected Receiver ===");
  Serial.println("Scanning for sender...\n");

  BLEDevice::init("ESP32-Receiver");

  // Start scan
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  // Connect if found
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("[BLE] Ready to receive data");
    } else {
      Serial.println("[ERROR] Connection failed");
    }
    doConnect = false;
  }

  // If disconnected, restart scan
  if (!connected && !doConnect) {
    delay(1000);
    Serial.println("[BLE] Restarting scan...");
    BLEDevice::getScan()->start(0);
  }

  delay(10);
}
