

#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "XiFi-Test-AP";
const char* password = "xifi2025";

const int UDP_PORT = 5000;

WiFiUDP udp;

uint32_t lastSeq = 0;
uint32_t packetsReceived = 0;
uint32_t packetsLost = 0;
bool firstPacket = true;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== UDP Unicast Receiver ===");

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  int retries = 40;
  while (WiFi.status() != WL_CONNECTED && retries-- > 0) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected!\n");
    Serial.printf("Receiver IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Listening on UDP port: %d\n\n", UDP_PORT);
  } else {
    Serial.println("WiFi connection failed!");
    return;
  }

  udp.begin(UDP_PORT);

  Serial.println("Waiting for packets...\n");
}

void loop() {
  int packetSize = udp.parsePacket();

  if (packetSize >= 4) {
    uint8_t buffer[20];
    int len = udp.read(buffer, 20);

    if (len >= 4) {
      uint32_t seq;
      memcpy(&seq, buffer, 4);

      if (!firstPacket) {
        uint32_t expected = lastSeq + 1;
        if (seq != expected) {
          uint32_t lost = seq - expected;
          packetsLost += lost;
          Serial.printf("[LOSS] Expected %u, got %u. Lost %u packets\n", expected, seq, lost);
        }
      } else {
        firstPacket = false;
      }

      lastSeq = seq;
      packetsReceived++;
      
      if (seq % 50 == 0) {
        float lossRate = (packetsReceived + packetsLost) > 0 ?
                         (packetsLost * 100.0) / (packetsReceived + packetsLost) : 0.0;
        Serial.printf("Received seq: %u | Total: %u | Lost: %u (%.2f%%)\n",
                      seq, packetsReceived, packetsLost, lossRate);
      }
    }
  }

  delay(1);
}
