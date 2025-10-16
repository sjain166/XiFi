#include <BLEDevice.h>      // Core BLE functionality
#include <BLEServer.h>      // Server-specific functions (like listen/accept in TCP)
#include <BLEUtils.h>       // Utility functions
#include <BLE2902.h>        // Descriptor for notifications (tells client we can push data)

// BLE Server
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;                    // The BLE server object (like server socket)
BLECharacteristic* pCharacteristic = NULL;    // The data endpoint (like a file handle)
bool deviceConnected = false;                 // Connection state flag
uint32_t packetCount = 0;                     // Counter for packets sent



class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();
      
      Serial.println("\n--- PACKET RECEIVED (onWrite callback) ---");
      Serial.print("Length: ");
      Serial.print(value.length());
      Serial.println(" bytes");
      
      if (value.length() > 0) {
        Serial.print("Data (ASCII): ");
        for (int i = 0; i < value.length(); i++) {
          Serial.print(value[i]);
        }
        Serial.println();
        Serial.print("Data (Hex):   ");
        for (int i = 0; i < value.length(); i++) {
          Serial.printf("%02X ", (uint8_t)value[i]);
        }
        Serial.println();
      }
      
    }
};


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;  // Set our flag
      Serial.println("\n=== CLIENT CONNECTED ===");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;  // Clear our flag
      Serial.println("\n=== CLIENT DISCONNECTED ===");
      BLEDevice::startAdvertising();
    }
};




void setup() {

  Serial.begin(115200);
  Serial.println("ESP32 BLE SERVER Starting...");

  BLEDevice::init("ESP32_BLE_Server");
  Serial.println("[Step 1] BLE Device initialized: ESP32_BLE_Server");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  Serial.println("[Step 3] BLE Service created");

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE  |  // Client can write
                      BLECharacteristic::PROPERTY_NOTIFY    // Server can notify
                    );

  
  pCharacteristic->addDescriptor(new BLE2902()); // Think of it as "metadata" that tells client how to subscribe to updates, Without this, client can't register for notifications

  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *bleAdvertising= BLEDevice::getAdvertising();

  bleAdvertising->addServiceUUID(SERVICE_UUID); // Advertise our service UUID
  bleAdvertising->setScanResponse(true);  // Don't need scan response for simple setup
  bleAdvertising->setMinPreferred(0x06);  // Negotiation
  bleAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("[Step 6] Advertising started");
  Serial.println("\n*** SERVER IS READY ***");
  Serial.println("Waiting for client to connect...\n");

}

void loop() {
  delay(500);  // Small delay to prevent busy-waiting
}
