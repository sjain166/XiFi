/*
 * ESP32-C5 DUAL PROTOCOL MEMORY TEST
 * ====================================
 * Tests if ESP32-C5 can load both WiFi and BLE stacks simultaneously
 * This will verify your memory upgrade solved the previous problem
 * 
 * EXPECTED OUTCOME:
 * - Should compile successfully (no memory overflow errors)
 * - Should print initialization success messages
 * - Should show available memory after loading both stacks
 * 
 * Upload to: ESP32-C5-WROOM-1
 */

#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";        // Replace with your WiFi SSID
const char* password = "YOUR_WIFI_PASSWORD"; // Replace with your WiFi password

// BLE Configuration
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

// Memory tracking
size_t heapAtStart = 0;
size_t heapAfterWiFi = 0;
size_t heapAfterBLE = 0;

// BLE Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("[BLE] Client connected!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("[BLE] Client disconnected");
      BLEDevice::startAdvertising();
    }
};

void printMemoryInfo(const char* stage) {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.print("║ Memory Status: ");
  Serial.print(stage);
  for(int i = strlen(stage); i < 23; i++) Serial.print(" ");
  Serial.println("║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.print("║ Free Heap:       ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes          ║");
  Serial.print("║ Total Heap:      ");
  Serial.print(ESP.getHeapSize());
  Serial.println(" bytes          ║");
  Serial.print("║ Min Free Heap:   ");
  Serial.print(ESP.getMinFreeHeap());
  Serial.println(" bytes          ║");
  
  // Calculate PSRAM if available
  if(ESP.getPsramSize() > 0) {
    Serial.print("║ PSRAM Total:     ");
    Serial.print(ESP.getPsramSize());
    Serial.println(" bytes          ║");
    Serial.print("║ PSRAM Free:      ");
    Serial.print(ESP.getFreePsram());
    Serial.println(" bytes          ║");
  } else {
    Serial.println("║ PSRAM:           Not available         ║");
  }
  Serial.println("╚════════════════════════════════════════╝\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize
  
  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════════════════╗");
  Serial.println("║   ESP32-C5 DUAL PROTOCOL MEMORY TEST              ║");
  Serial.println("║   Testing WiFi + BLE Simultaneous Loading         ║");
  Serial.println("╚═══════════════════════════════════════════════════╝\n");
  
  // Print chip information
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("CHIP INFORMATION:");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.print("Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Chip Revision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("Number of Cores: ");
  Serial.println(ESP.getChipCores());
  Serial.print("CPU Frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  
  // Measure initial memory
  heapAtStart = ESP.getFreeHeap();
  printMemoryInfo("INITIAL (No protocols)");
  
  // ============ TEST 1: Initialize WiFi ============
  Serial.println("\n▶ TEST 1: Loading WiFi Stack...");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  
  WiFi.mode(WIFI_STA);
  Serial.println("✓ WiFi mode set to STA");
  
  WiFi.begin(ssid, password);
  Serial.print("  Connecting to WiFi");
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi Connected!");
    Serial.print("  IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("  Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("⚠ WiFi Connection Failed (but stack loaded)");
    Serial.println("  This is OK for memory testing");
  }
  
  heapAfterWiFi = ESP.getFreeHeap();
  size_t wifiMemoryUsed = heapAtStart - heapAfterWiFi;
  Serial.print("  WiFi Stack Memory Used: ");
  Serial.print(wifiMemoryUsed);
  Serial.println(" bytes");
  
  printMemoryInfo("AFTER WiFi");
  
  // ============ TEST 2: Initialize BLE ============
  Serial.println("\n▶ TEST 2: Loading BLE Stack...");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  
  BLEDevice::init("ESP32-C5-Test");
  Serial.println("✓ BLE Device initialized");
  
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  Serial.println("✓ BLE Server created");
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  Serial.println("✓ BLE Service created");
  
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  Serial.println("✓ BLE Characteristic created");
  
  pService->start();
  Serial.println("✓ BLE Service started");
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("✓ BLE Advertising started");
  Serial.println("  Device Name: ESP32-C5-Test");
  
  heapAfterBLE = ESP.getFreeHeap();
  size_t bleMemoryUsed = heapAfterWiFi - heapAfterBLE;
  Serial.print("  BLE Stack Memory Used: ");
  Serial.print(bleMemoryUsed);
  Serial.println(" bytes");
  
  printMemoryInfo("AFTER WiFi + BLE");
  
  // ============ FINAL RESULTS ============
  Serial.println("\n╔═══════════════════════════════════════════════════╗");
  Serial.println("║              MEMORY TEST RESULTS                  ║");
  Serial.println("╚═══════════════════════════════════════════════════╝");
  
  size_t totalUsed = heapAtStart - heapAfterBLE;
  float percentUsed = ((float)totalUsed / (float)heapAtStart) * 100.0;
  
  Serial.println("\n┌─────────────────────────────────────────────────┐");
  Serial.println("│ MEMORY USAGE BREAKDOWN:                         │");
  Serial.println("├─────────────────────────────────────────────────┤");
  Serial.print("│ Initial Free Heap:     ");
  Serial.print(heapAtStart);
  Serial.println(" bytes");
  Serial.print("│ WiFi Stack Used:       ");
  Serial.print(wifiMemoryUsed);
  Serial.println(" bytes");
  Serial.print("│ BLE Stack Used:        ");
  Serial.print(bleMemoryUsed);
  Serial.println(" bytes");
  Serial.print("│ Total Used:            ");
  Serial.print(totalUsed);
  Serial.println(" bytes");
  Serial.print("│ Remaining Free Heap:   ");
  Serial.print(heapAfterBLE);
  Serial.println(" bytes");
  Serial.print("│ Percentage Used:       ");
  Serial.print(percentUsed, 2);
  Serial.println("%");
  Serial.println("└─────────────────────────────────────────────────┘\n");
  
  // Determine test result
  if (heapAfterBLE > 50000) {
    Serial.println("╔═══════════════════════════════════════════════════╗");
    Serial.println("║                 ✓✓✓ TEST PASSED ✓✓✓              ║");
    Serial.println("╠═══════════════════════════════════════════════════╣");
    Serial.println("║ ESP32-C5 successfully loaded BOTH WiFi and BLE!   ║");
    Serial.println("║ Sufficient memory remains for application code.   ║");
    Serial.println("║                                                    ║");
    Serial.println("║ You can proceed with your dual-protocol project!  ║");
    Serial.println("╚═══════════════════════════════════════════════════╝");
  } else if (heapAfterBLE > 20000) {
    Serial.println("╔═══════════════════════════════════════════════════╗");
    Serial.println("║               ⚠ TEST WARNING ⚠                    ║");
    Serial.println("╠═══════════════════════════════════════════════════╣");
    Serial.println("║ Both stacks loaded, but memory is tight.          ║");
    Serial.println("║ You may need to optimize your application code.   ║");
    Serial.println("╚═══════════════════════════════════════════════════╝");
  } else {
    Serial.println("╔═══════════════════════════════════════════════════╗");
    Serial.println("║                ✗✗✗ TEST FAILED ✗✗✗               ║");
    Serial.println("╠═══════════════════════════════════════════════════╣");
    Serial.println("║ Memory critically low after loading both stacks!  ║");
    Serial.println("║ Consider optimization or alternative approach.    ║");
    Serial.println("╚═══════════════════════════════════════════════════╝");
  }
  
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("TEST COMPLETE - Monitor will show live status");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  // Show live status every 5 seconds
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 5000) {
    lastUpdate = millis();
    
    Serial.println("┌─────────────────────────────────────┐");
    Serial.println("│         LIVE STATUS UPDATE          │");
    Serial.println("├─────────────────────────────────────┤");
    
    // WiFi Status
    Serial.print("│ WiFi: ");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected (");
      Serial.print(WiFi.RSSI());
      Serial.print(" dBm)");
    } else {
      Serial.print("Disconnected");
    }
    for(int i=0; i<20; i++) Serial.print(" ");
    Serial.println("│");
    
    // BLE Status
    Serial.print("│ BLE:  ");
    if (deviceConnected) {
      Serial.print("Client Connected");
    } else {
      Serial.print("Advertising");
    }
    for(int i=0; i<20; i++) Serial.print(" ");
    Serial.println("│");
    
    // Memory Status
    Serial.print("│ Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" bytes");
    for(int i=0; i<15; i++) Serial.print(" ");
    Serial.println("│");
    
    Serial.println("└─────────────────────────────────────┘\n");
  }
  
  // If BLE client is connected, send periodic updates
  if (deviceConnected) {
    static unsigned long lastNotify = 0;
    if (millis() - lastNotify > 2000) {
      lastNotify = millis();
      String message = "Heap: " + String(ESP.getFreeHeap()) + " bytes";
      pCharacteristic->setValue(message.c_str());
      pCharacteristic->notify();
    }
  }
}