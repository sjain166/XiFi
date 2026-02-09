#include <WiFi.h>

const char* AP_SSID = "XiFi-Test-AP";
const char* AP_PASSWORD = "xifi2025";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.printf("[WiFi] Creating AP: %s\n", AP_SSID);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  IPAddress IP = WiFi.softAPIP();
  Serial.printf("[WiFi] AP IP: %s\n", IP.toString().c_str());
  Serial.printf("[WiFi] Password: %s\n", AP_PASSWORD);
  Serial.println("\n[Ready] AP is running. Devices can connect.\n");
}

void loop() {
  delay(1000);
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    Serial.printf("[Info] Connected clients: %d\n", WiFi.softAPgetStationNum());
    lastPrint = millis();
  }
}
