#include <WiFi.h>

const char* ssid = "NETGEAR73";      // Your WiFi network name
const char* password = "rockybird723"; // Your WiFi password


const char* serverIP = "192.168.1.35";  // Replace with server's IP address
const uint16_t serverPort = 8080;

WiFiClient client;

bool connected = false;

uint16_t txPacketCount = 0;


void connectToServer() { 
  if (client.connect(serverIP, serverPort)) {
    connected = true;
    Serial.println("║     TCP CONNECTION ESTABLISHED         ║");
    Serial.print("Server IP: ");
    Serial.println(serverIP);
    Serial.print("Server Port: ");
    Serial.println(serverPort);
    
  } else {
    Serial.println("Connection failed!");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("║   ESP32 WiFi TCP Client   ║");
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

}

void loop() {

  // Try to connect if not connected
  if (!connected) {
    connectToServer();
    delay(5000);  // Wait 5 seconds before retry
    return;
  }

  txPacketCount++;
  char message[100];
  sprintf(message, "Client packet #%lu\n", txPacketCount);

  // Send via TCP (this is the send() you know!)
  client.print(message);

  Serial.println("\n--- Payload (ASCII) ---");
  Serial.print(message);

  delay(5000);

}
