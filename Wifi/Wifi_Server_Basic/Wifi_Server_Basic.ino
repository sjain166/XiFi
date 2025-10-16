#include <WiFi.h>

const char* ssid = "NETGEAR73";      // Your WiFi network name
const char* password = "rockybird723"; // Your WiFi password

const uint16_t port = 8080;  
WiFiServer server(port);     
WiFiClient client;          

bool clientConnected = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  server.begin();
  Serial.print("\nTCP Server started on port ");
  Serial.println(port);
  Serial.println("\n>>> Waiting for client connection <<<\n");

}

void loop() {
  
  // ========== ACCEPT NEW CONNECTIONS ========== 
  // THIS WAS MISSING!
  if (!clientConnected) {
    client = server.available();  // Check for new client
    
    if (client) {
      clientConnected = true;
      Serial.println("\n=== CLIENT CONNECTED ===");
      Serial.print("Client IP: ");
      Serial.println(client.remoteIP());
      Serial.print("Client Port: ");
      Serial.println(client.remotePort());
      Serial.println();
    }
  }
  
  // ========== HANDLE CONNECTED CLIENT ==========
  if (clientConnected) {
    
    // Check if client disconnected
    if (!client.connected()) {
      Serial.println("\n=== CLIENT DISCONNECTED ===\n");
      client.stop();
      clientConnected = false;
      return;
    }
    
    // Read data from client
    if (client.available()) {
      String data = client.readStringUntil('\n');
      
      Serial.println("\n--- RECEIVED FROM CLIENT ---");
      Serial.print("Length: ");
      Serial.print(data.length());
      Serial.println(" bytes");
      
      Serial.print("Source IP: ");
      Serial.println(client.remoteIP());
      
      Serial.print("Source Port: ");
      Serial.println(client.remotePort());
      
      Serial.print("Dest Port: ");
      Serial.println(port);
      
      Serial.println("\n--- Payload (ASCII) ---");
      Serial.println(data);
      Serial.println("---------------------------\n");
    }
  }

  delay(10);
}
