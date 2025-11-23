// XiFi Power Logger - Serial control + optional web download

#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_INA219.h>

#define I2C_SDA 7
#define I2C_SCL 6
#define MARKER_PIN 5
#define SAMPLE_INTERVAL_MS 50
#define BUFFER_SIZE 6000

const char* WIFI_SSID = "XiFi-Peer";
const char* WIFI_PASS = "xifi1234";

Adafruit_INA219 ina219;
WebServer server(80);

struct Sample {
  unsigned long timestamp;
  float voltage;
  float current;
  float power;
  uint8_t marker;
};

Sample buffer[BUFFER_SIZE];
int sampleCount = 0;
unsigned long lastSample = 0;
unsigned long lastStatus = 0;
bool logging = false;
unsigned long logStartTime = 0;
Sample latest;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== XiFi Power Logger ===");
  Serial.println("Commands: start, stop, status, csv");

  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(MARKER_PIN, INPUT_PULLDOWN);

  if (!ina219.begin()) {
    Serial.println("INA219 not found!");
    while (1) delay(1000);
  }
  ina219.setCalibration_16V_400mA();
  Serial.println("INA219 ready");

  // WiFi optional - don't block if fails
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  int timeout = 20;
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWeb: http://%s/csv\n", WiFi.localIP().toString().c_str());
    server.on("/csv", handleCSV);
    server.begin();
  } else {
    Serial.println("\nWiFi failed - use 'csv' command for Serial output");
  }

  Serial.println("Ready. Type 'start' to begin logging.\n");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }

  // Sample INA219
  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = now;

    latest.timestamp = now;
    latest.voltage = ina219.getBusVoltage_V();
    latest.current = abs(ina219.getCurrent_mA());
    latest.power = abs(ina219.getPower_mW());
    latest.marker = digitalRead(MARKER_PIN);

    if (logging && sampleCount < BUFFER_SIZE) {
      buffer[sampleCount] = latest;
      sampleCount++;
    }
  }

  // Print status every 2 seconds while logging
  if (logging && now - lastStatus >= 2000) {
    lastStatus = now;
    Serial.printf("[%d] %.2fmA %.2fmW marker=%d\n",
      sampleCount, latest.current, latest.power, latest.marker);
  }

  // Handle Serial commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "start") {
      sampleCount = 0;
      logStartTime = millis();
      logging = true;
      Serial.println(">>> LOGGING STARTED <<<");
    }
    else if (cmd == "stop") {
      logging = false;
      Serial.printf(">>> STOPPED - %d samples <<<\n", sampleCount);
    }
    else if (cmd == "status") {
      Serial.printf("Logging: %s, Samples: %d/%d\n",
        logging?"YES":"NO", sampleCount, BUFFER_SIZE);
      Serial.printf("Current: %.2fmA, Power: %.2fmW, Marker: %d\n",
        latest.current, latest.power, latest.marker);
    }
    else if (cmd == "csv") {
      printCSV();
    }
  }
}

void printCSV() {
  Serial.println("\n>>> CSV START <<<");
  Serial.println("timestamp_ms,voltage_V,current_mA,power_mW,marker");
  for (int i = 0; i < sampleCount; i++) {
    Serial.printf("%lu,%.3f,%.2f,%.2f,%d\n",
      buffer[i].timestamp - logStartTime,
      buffer[i].voltage,
      buffer[i].current,
      buffer[i].power,
      buffer[i].marker);
  }
  Serial.println(">>> CSV END <<<");
}

void handleCSV() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  server.sendContent("timestamp_ms,voltage_V,current_mA,power_mW,marker\n");
  for (int i = 0; i < sampleCount; i++) {
    String line = String(buffer[i].timestamp - logStartTime) + "," +
                  String(buffer[i].voltage, 3) + "," +
                  String(buffer[i].current, 2) + "," +
                  String(buffer[i].power, 2) + "," +
                  String(buffer[i].marker) + "\n";
    server.sendContent(line);
  }
}
