#include <Wire.h>
#include <Adafruit_INA219.h>

#define I2C_SDA 7
#define I2C_SCL 6

#define MARKER_BASELINE_PIN  27
#define MARKER_BULK_PIN      4
#define MARKER_IDLE_PIN      5

#define SAMPLE_INTERVAL_MS   0.1

Adafruit_INA219 ina219;

unsigned long lastSample = 0;
bool logging = false;
unsigned long logStartTime = 0;
unsigned long sampleCount = 0;

struct Sample {
  unsigned long timestamp;
  float voltage;
  float current;
  float power;
  uint8_t marker_baseline;
  uint8_t marker_bulk;
  uint8_t marker_idle;
} latest;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Power Collector --> Commands: start, stop");

  Wire.begin(I2C_SDA, I2C_SCL);

  pinMode(MARKER_BASELINE_PIN, INPUT_PULLDOWN);
  pinMode(MARKER_BULK_PIN, INPUT_PULLDOWN);
  pinMode(MARKER_IDLE_PIN, INPUT_PULLDOWN);


  if (!ina219.begin()) {
    Serial.println("INA219 not found!");
    while (1) delay(1000);
  }

  ina219.setCalibration_16V_400mA();
  Serial.println("Ready");

}

void loop() {
  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = now;

    latest.timestamp = now;
    latest.voltage = ina219.getBusVoltage_V();
    latest.current = abs(ina219.getCurrent_mA());
    latest.power = abs(ina219.getPower_mW());
    latest.marker_baseline = digitalRead(MARKER_BASELINE_PIN);
    latest.marker_bulk = digitalRead(MARKER_BULK_PIN);
    latest.marker_idle = digitalRead(MARKER_IDLE_PIN);

    if (logging) {
      Serial.printf("DATA,%lu,%.3f,%.2f,%.2f,%d,%d,%d\n",
        latest.timestamp - logStartTime,
        latest.voltage,
        latest.current,
        latest.power,
        latest.marker_baseline,
        latest.marker_bulk,
        latest.marker_idle);

      sampleCount++;
    }
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "start") {
      sampleCount = 0;
      logStartTime = millis();
      logging = true;
      Serial.println("\nLOGGING STARTED!!!");
    }
    else if (cmd == "stop") {
      logging = false;
      Serial.printf("\nSTOPPED !!! - %lu samples\n", sampleCount);
    }
    else {
      Serial.println("ERROR !!! :(");
    }
  }
}
