// DUT ESP - Single test mode. Change TEST_MODE and PAYLOAD_SIZE, flash, run.

#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEClient.h>

// >>> CHANGE THESE BEFORE EACH FLASH <<<
#define TEST_MODE 3          // 1=BLE_ADV, 2=BLE_CONN, 3=WIFI
#define PAYLOAD_SIZE 50       // 1, 2, 10, 50, 100, 512, 1024

#define ITERATIONS 100
#define DELAY_BETWEEN_MS 50
#define MARKER_PIN 4  // Connect to Logger's marker input

const char* PEER_WIFI_SSID = "XiFi-Peer";
const char* PEER_WIFI_PASS = "xifi1234";
const char* PEER_IP = "192.168.4.1";
#define PEER_TCP_PORT 8080

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

uint8_t payload[1024];
BLEClient* pClient = NULL;
BLERemoteCharacteristic* pRemoteChar = NULL;
WiFiClient tcpClient;
bool setupDone = false;

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(MARKER_PIN, OUTPUT);
  digitalWrite(MARKER_PIN, LOW);

  for (int i = 0; i < PAYLOAD_SIZE; i++) payload[i] = i & 0xFF;

  const char* modeName = TEST_MODE==1 ? "BLE_ADV" : TEST_MODE==2 ? "BLE_CONN" : "WIFI";
  Serial.printf("\n[TEST] %s %d bytes\n", modeName, PAYLOAD_SIZE);
  Serial.println(">>> START LOGGING NOW! Test begins in 5s <<<");
  delay(5000);

  if (TEST_MODE == 1) setupBleAdv();
  else if (TEST_MODE == 2) setupBleConn();
  else if (TEST_MODE == 3) setupWifi();

  setupDone = true;
  Serial.println("[START]");
  digitalWrite(MARKER_PIN, HIGH);  // Marker ON = transfer phase
}

void loop() {
  static int iteration = 0;
  if (!setupDone) return;

  if (iteration < ITERATIONS) {
    if (TEST_MODE == 1) sendBleAdv();
    else if (TEST_MODE == 2) sendBleConn();
    else if (TEST_MODE == 3) sendWifi();

    iteration++;
    delay(DELAY_BETWEEN_MS);
  } else if (iteration == ITERATIONS) {
    digitalWrite(MARKER_PIN, LOW);  // Marker OFF = transfer done
    Serial.println("[END]");
    const char* modeName = TEST_MODE==1 ? "BLE_ADV" : TEST_MODE==2 ? "BLE_CONN" : "WIFI";
    Serial.printf("[DONE] %s,%d bytes,%d iterations\n", modeName, PAYLOAD_SIZE, ITERATIONS);
    Serial.println(">>> STOP LOGGING - DOWNLOAD CSV <<<");
    iteration++;
  }
}

// BLE Advertising
void setupBleAdv() {
  BLEDevice::init("XiFi-DUT");
}

void sendBleAdv() {
  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  pAdv->stop();

  int remaining = PAYLOAD_SIZE;
  int offset = 0;
  while (remaining > 0) {
    BLEAdvertisementData advData;
    int chunk = min(remaining, 28);
    String data;
    for (int i = 0; i < chunk; i++) data += (char)payload[offset + i];
    advData.setManufacturerData(data);
    pAdv->setAdvertisementData(advData);
    pAdv->start();
    delay(20);
    pAdv->stop();
    remaining -= chunk;
    offset += chunk;
  }
}

// BLE Connected
void setupBleConn() {
  BLEDevice::init("XiFi-DUT");
  pClient = BLEDevice::createClient();

  Serial.println("Scanning...");
  BLEScan* pScan = BLEDevice::getScan();
  pScan->setActiveScan(true);
  BLEScanResults* results = pScan->start(5);

  BLEAdvertisedDevice* peer = nullptr;
  for (int i = 0; i < results->getCount(); i++) {
    BLEAdvertisedDevice dev = results->getDevice(i);
    if (dev.haveServiceUUID() && dev.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      peer = new BLEAdvertisedDevice(dev);
      break;
    }
  }

  if (!peer) {
    Serial.println("Peer not found!");
    while(1) delay(1000);
  }

  pClient->connect(peer);
  BLERemoteService* pService = pClient->getService(SERVICE_UUID);
  pRemoteChar = pService->getCharacteristic(CHARACTERISTIC_UUID);
  Serial.println("BLE Connected");
}

void sendBleConn() {
  if (pRemoteChar) pRemoteChar->writeValue(payload, PAYLOAD_SIZE, false);
}

// WiFi
void setupWifi() {
  WiFi.begin(PEER_WIFI_SSID, PEER_WIFI_PASS);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" OK");

  tcpClient.connect(PEER_IP, PEER_TCP_PORT);
  Serial.println("TCP Connected");
}

void sendWifi() {
  if (!tcpClient.connected()) tcpClient.connect(PEER_IP, PEER_TCP_PORT);
  tcpClient.write(payload, PAYLOAD_SIZE);
  tcpClient.flush();  // Ensure data is sent
}
