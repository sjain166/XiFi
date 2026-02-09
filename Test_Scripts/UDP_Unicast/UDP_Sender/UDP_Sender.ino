
#include <WiFi.h>
#include <WiFiUdp.h>

#define SEND_INTERVAL_MS  100   // Options: 20, 50, 100


const char* ssid = "XiFi-Test-AP";
const char* password = "xifi2025";

IPAddress receiverIP(192,168,4,2);  
const int UDP_PORT = 5000;

WiFiUDP udp;
uint32_t sequenceNumber = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.printf("Send interval: %d ms\n", SEND_INTERVAL_MS);

  WiFi.begin(ssid, password);

  int retries = 40;
  while (WiFi.status() != WL_CONNECTED && retries-- > 0) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected!\n");
  } else {
    Serial.println("WiFi connection failed!");
    return;
  }

  udp.begin(UDP_PORT);
}

void loop() {
  static unsigned long lastSendTime = 0;
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected!");
    delay(1000);
    return;
  }

  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;

    uint8_t payload[20];
    memcpy(payload, &sequenceNumber, 4);
    memcpy(payload + 4, &now, 4);

    for (int i = 8; i < 20; i++) {
      payload[i] = (i - 8) & 0xFF;
    }

    udp.beginPacket(receiverIP, UDP_PORT);
    udp.write(payload, 20);
    udp.endPacket();

    if (sequenceNumber == 0 || sequenceNumber % 50 == 0) {
      Serial.printf("Sent seq: %u\n", sequenceNumber);
    }

    sequenceNumber++;
  }

  delay(1);
}
