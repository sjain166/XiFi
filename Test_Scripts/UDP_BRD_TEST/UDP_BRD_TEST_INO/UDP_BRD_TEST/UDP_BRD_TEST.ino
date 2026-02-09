#include <WiFi.h>
#include <WiFiUdp.h>

const char *ssid = "XiFi-Test-AP";
const char *password = "xifi2025";

WiFiUDP udp;

const int DELAY_TIME_MS = 40;

const int SEND_RATE = 20;                  // bytes per second
const int BROADCAST_PORT = 5000;            // receiver listens on this port

uint8_t buffer[SEND_RATE];
uint32_t sequenceNumber = 0;

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retries = 20;
  while (WiFi.status() != WL_CONNECTED && retries > 0) {
    Serial.print(".");
    delay(500);
    retries--;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect.");
    return;
  }

  udp.begin(BROADCAST_PORT);
  for (int i = 0; i < SEND_RATE; i++) {
    buffer[i] = i & 0xFF;
  }

  Serial.println("Broadcast sender ready.");
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Lost WiFi!");
    return;
  }

  // Pack sequence number into first 4 bytes (little-endian)
  buffer[0] = sequenceNumber & 0xFF;
  buffer[1] = (sequenceNumber >> 8) & 0xFF;
  buffer[2] = (sequenceNumber >> 16) & 0xFF;
  buffer[3] = (sequenceNumber >> 24) & 0xFF;
  
  udp.beginPacket("255.255.255.255", BROADCAST_PORT);
  udp.write(buffer, SEND_RATE);
  udp.endPacket();

  if (sequenceNumber % 50 == 0) {
      Serial.print("Sent seq:");
      Serial.print(sequenceNumber);
      Serial.print(" (");
      Serial.print(SEND_RATE);
      Serial.println(" bytes)");
  }


  sequenceNumber++;

  delay(DELAY_TIME_MS);
}