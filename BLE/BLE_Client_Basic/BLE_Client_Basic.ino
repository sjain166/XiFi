
#include <BLEDevice.h>      // Core BLE functionality
#include <BLEClient.h>      // Client-specific functions (like connect() in TCP)
#include <BLEUtils.h>       // Utility functions
#include <BLEScan.h>        // Scanning functionality (discovery)
#include <BLE2902.h>        // Descriptor for notifications

// BLE Client

// This information exchnage happen during the scanning, hard-coded it right now to avoid delay
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

static BLEAddress *pServerAddress;                    // Server's MAC address (like IP address)
static boolean doConnect = false;                      // Flag: should we connect?
static boolean connected = false;                      // Flag: are we connected?
static BLERemoteCharacteristic* pRemoteCharacteristic; // Pointer to server's characteristic
static BLEClient* pClient;                            // Our client object
uint32_t packetCount = 0;                             // Counter for packets sent


// On Each Device Found, this function is triggered and it checks for the server SERVICE_UUID to connect to
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && 
        advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
      Serial.println("[Scan] *** FOUND OUR SERVER! ***");      
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    }
  }
};

class MyClientCallback : public BLEClientCallbacks {
  
  void onConnect(BLEClient* pclient) {
    Serial.println("\n=== CONNECTED TO SERVER ===");
    Serial.println("BLE link layer completed pairing/connection setup");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("\n=== DISCONNECTED FROM SERVER ===");  }
};


bool connectToServer(BLEAddress pAddress) {


    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(pAddress);
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    Serial.println("[Step 3] Found our service on server");
    Serial.println(SERVICE_UUID);


    pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("FAILED to find characteristic UUID: ");
      Serial.println(CHARACTERISTIC_UUID);
      Serial.println("This is like requesting a resource that doesn't exist");
      return false;
    }


    Serial.println("[Step 4] Found characteristic");
    Serial.print("         Characteristic UUID: ");
    Serial.println(CHARACTERISTIC_UUID);
    Serial.println("         This is our data channel");

    connected = true;
    Serial.println("\n*** CLIENT CONNECTED SUCCESSFULLY ***");
    Serial.println("We can now send data!\n");
    return true;
}



void setup() {
  Serial.begin(115200);
  Serial.println("[STEP1] ESP32 BLE CLIENT Starting...");
  
  BLEDevice::init("ESP32_BLE_Client");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

  pBLEScan->setActiveScan(true); // Active scanning gets more info
  
  Serial.println("[Step 2] Starting BLE scan...");

  // Start scanning for 30 seconds
  // Callbacks will be triggered for each device found
  pBLEScan->start(30);

}

void loop() {

  if (doConnect == true) {
    Serial.println("Attempting to connect to server..."); 
    if (connectToServer(*pServerAddress)) {
      Serial.println("============Connection successful!===============");
    } else {
      Serial.println(">>> Connection failed!!!!!!!");
      delay(5000);
    }
    doConnect = false;  // Clear flag
  }

  if (connected) {
    packetCount++;
    
    // Create message to send
    char message[50];
    sprintf(message, "Client packet #%lu", packetCount);
    
    pRemoteCharacteristic->writeValue(message, strlen(message));
    
    Serial.println("\n--- PACKET SENT (writeValue) ---");
    Serial.print("Data: ");
    Serial.println(message);
    Serial.print("Length: ");
    Serial.print(strlen(message));
    Serial.println(" bytes");
    Serial.println("--------------------------------");
    
    delay(5000); 
  }

  delay(100);  // Small delay to prevent busy-waiting

}
