// Peer ESP - BLE Server + WiFi AP/Server for DUT to send data to

#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// WiFi AP settings
const char* AP_SSID = "XiFi-Peer";
const char* AP_PASS = "xifi1234";
WiFiServer tcpServer(8080);

// BLE settings
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool bleConnected = false;
unsigned long bleRxBytes = 0;
unsigned long wifiRxBytes = 0;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    bleConnected = true;
    Serial.println("BLE: Client connected");
  }
  void onDisconnect(BLEServer* pServer) {
    bleConnected = false;
    Serial.println("BLE: Client disconnected");
    pServer->startAdvertising();
  }
};

class CharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    size_t len = pChar->getLength();
    bleRxBytes += len;
    Serial.printf("BLE RX: %d bytes (total: %lu)\n", len, bleRxBytes);
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Peer ESP: BLE + WiFi Server ===");

  // Start WiFi AP
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("WiFi AP: %s (pass: %s)\n", AP_SSID, AP_PASS);
  Serial.printf("WiFi IP: %s\n", WiFi.softAPIP().toString().c_str());
  tcpServer.begin();
  Serial.println("TCP server on port 8080");

  // Start BLE
  BLEDevice::init("XiFi-Peer");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pCharacteristic->setCallbacks(new CharCallbacks());
  pService->start();

  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->setScanResponse(true);
  pAdv->start();
  Serial.println("BLE advertising started");

  Serial.println("\nReady to receive data from DUT");
}

void loop() {
  // Handle WiFi TCP clients
  WiFiClient client = tcpServer.available();
  if (client) {
    Serial.println("WiFi: Client connected");
    while (client.connected()) {
      while (client.available()) {
        uint8_t buf[2048];
        size_t read = client.read(buf, sizeof(buf));
        wifiRxBytes += read;
      }
      yield();  // Let system breathe
    }
    Serial.printf("WiFi RX: total %lu bytes\n", wifiRxBytes);
    client.stop();
    Serial.println("WiFi: Client disconnected");
  }
  delay(1);
}
