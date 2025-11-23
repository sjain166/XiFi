# XiFi Power Measurement - Hardware Setup and Guidelines

## Table of Contents
1. [Overview](#overview)
2. [Hardware Components](#hardware-components)
3. [Measurement Configuration Options](#measurement-configuration-options)
4. [Recommended Setup: Dual Independent Measurement](#recommended-setup-dual-independent-measurement)
5. [Wiring Diagrams](#wiring-diagrams)
6. [INA219 Specifications](#ina219-specifications)
7. [Calibration Procedure](#calibration-procedure)
8. [Test Protocol](#test-protocol)
9. [Accuracy Guidelines](#accuracy-guidelines)
10. [Safety Considerations](#safety-considerations)
11. [Troubleshooting](#troubleshooting)

---

## Overview

This document describes the hardware setup for measuring power consumption of WiFi and BLE operations on the ESP32-C5 microcontroller. The goal is to characterize power usage for different wireless operations to inform an intelligent routing algorithm.

### Measurement Goals
- **BLE Advertisement**: Power during advertising (different intervals, TX power)
- **BLE Connection Lifecycle**: Connection, idle, disconnection, reconnection costs
- **BLE Data Transfer**: Per-byte power cost for different payload sizes
- **WiFi Connection Lifecycle**: Association, DHCP, idle, disconnection costs
- **WiFi Data Transfer**: Per-byte power cost for different packet sizes
- **Idle States**: Baseline power for sleep modes and idle with radios active
- **Crossover Analysis**: Find optimal protocol selection based on payload size

---

## Hardware Components

### Required Equipment
- **4× ESP32-C5 Development Boards** (WROOM-1 module)
- **2× INA219 I2C Current/Power Sensor Modules** (HiLetgo or equivalent)
- **1× Bench Power Supply** (3.3V, capable of 500mA+ per ESP32)
- **1× Computer** with 2-4 USB ports (for Serial logging)
- **Jumper wires** (Female-Female recommended)
- **Breadboards** (optional, for cleaner setup)

### Optional Equipment
- **Keysight EDU34450A Multimeter** (for verification/calibration)
- **USB Power Meter** (for quick sanity checks)
- **Oscilloscope** (for visualizing current spikes)

### Software Requirements
- **Arduino IDE** or **PlatformIO** (for ESP32 programming)
- **Python 3.8+** with `pyserial` library (for data logging)
- **INA219 Arduino Library** (Adafruit INA219 recommended)

---

## Measurement Configuration Options

### Option 1: Self-Measurement (1 ESP + 1 INA) ❌ Not Recommended

```
Power Supply → INA219 → ESP32 #1 (runs test + reads INA via I2C)
```

**Pros:**
- Simple wiring
- Only uses 1 ESP32

**Cons:**
- ⚠️ **Measurement affects the measurement**: I2C polling adds 5-10mA overhead
- Cannot measure true idle or sleep states (CPU must stay active for I2C)
- Less accurate (~85-90%)

**Verdict:** Not suitable for scientific measurements

---

### Option 2: Dedicated Logger (2 ESP + 1 INA) ✅ Good

```
Power Supply → INA219 → ESP32 #1 (DUT - Device Under Test)
                  ↓
              ESP32 #2 (reads INA via I2C, logs to Serial)
```

**Pros:**
- DUT runs clean test code without overhead
- Can measure all power states including sleep
- Good accuracy (~95%)

**Cons:**
- No cross-validation
- Single point of failure (if INA is faulty)

**Verdict:** Good for quick/single tests

---

### Option 3: Dual Independent Measurement ✅✅ RECOMMENDED

```
Setup A: Power Supply → INA219 #1 → ESP32 #1 (DUT-A)
                           ↓
                       ESP32 #2 (Logger-A)

Setup B: Power Supply → INA219 #2 → ESP32 #3 (DUT-B)
                           ↓
                       ESP32 #4 (Logger-B)
```

**Pros:**
- ✅ **Cross-validation**: Run same test on both setups, average results
- ✅ **Statistical confidence**: Standard deviation shows measurement quality
- ✅ **Redundancy**: If one INA fails, other continues
- ✅ **Parallel testing**: Run BLE and WiFi tests simultaneously
- ✅ **Highest accuracy**: ~99% with averaging

**Cons:**
- More complex wiring
- Requires all 4 ESP32s

**Verdict:** Best for scientific/research measurements

---

### Option 4: Differential Measurement 🔬 Most Accurate (Advanced)

```
ESP32 #1 (DUT) ← INA219 #1 (measures total current)
     ↓
Peripheral ← INA219 #2 (measures peripheral only)
     ↓
ESP32 #2 (Logger - reads both INAs)

ESP32 current = INA219 #1 - INA219 #2
```

**Pros:**
- Isolates ESP32 power from peripheral noise
- Scientific-grade accuracy (~99.5%)

**Cons:**
- Complex wiring
- Requires careful shunt resistor selection
- Overkill for most use cases

**Verdict:** Only if extreme precision needed

---

## Recommended Setup: Dual Independent Measurement

### Hardware Configuration

#### Setup A (Primary Test Rig)

**ESP32 #1 (DUT-A):**
- Runs power test scripts (BLE, WiFi, idle tests)
- No measurement code - clean execution
- Powered through INA219 #1

**ESP32 #2 (Logger-A):**
- Reads INA219 #1 via I2C
- Logs power data to Serial
- Powered independently (USB or separate supply)

**INA219 #1:**
- Measures current/voltage for ESP32 #1
- I2C address: 0x40 (default)

#### Setup B (Verification Rig)

**ESP32 #3 (DUT-B):**
- Runs identical test scripts as DUT-A
- Powered through INA219 #2

**ESP32 #4 (Logger-B):**
- Reads INA219 #2 via I2C
- Logs power data to Serial
- Powered independently

**INA219 #2:**
- Measures current/voltage for ESP32 #3
- I2C address: 0x40 (or 0x41 if sharing I2C bus)

---

## Wiring Diagrams

### Setup A - Detailed Wiring

```
┌─────────────────────────────────────────────────────────────┐
│                       POWER SUPPLY                          │
│                     (3.3V, 500mA+)                          │
└────┬────────────────────────────────────────────┬───────────┘
     │                                            │
     │                                            │
     ▼                                            ▼
┌─────────────┐                            ┌──────────┐
│  INA219 #1  │                            │  ESP32   │
│             │                            │    #2    │
│ VIN+ ◄──────┤ (from PSU +3.3V)          │ (Logger) │
│ VIN- ───────┼──────────┐                │          │
│ VCC  ◄──────┤ +3.3V    │                │  VCC─────┤ USB/PSU
│ GND  ───────┤ GND      │                │  GND─────┤ GND
│ SDA  ───────┼──────────┼────────────────┤► GPIO8   │
│ SCL  ───────┼──────────┼────────────────┤► GPIO9   │
└─────────────┘          │                └──────────┘
                         │
                         ▼
                    ┌──────────┐
                    │  ESP32   │
                    │    #1    │
                    │  (DUT)   │
                    │          │
                    │  VCC ◄───┤ (from INA219 VIN-)
                    │  GND ◄───┤ (to PSU GND)
                    └──────────┘
```

### Pin Connections Summary

#### INA219 #1 → Power Supply
- **VCC**: +3.3V (powers the INA219 chip)
- **GND**: Ground
- **VIN+**: +3.3V (high side of shunt resistor)
- **VIN-**: To ESP32 #1 VCC (low side of shunt resistor)

#### INA219 #1 → ESP32 #2 (Logger)
- **SDA**: GPIO 8 (default I2C SDA on ESP32-C5)
- **SCL**: GPIO 9 (default I2C SCL on ESP32-C5)

#### ESP32 #1 (DUT)
- **VCC**: From INA219 VIN-
- **GND**: To Power Supply GND

#### ESP32 #2 (Logger)
- **VCC**: USB or separate 3.3V supply
- **GND**: Common ground with power supply
- **GPIO 8**: I2C SDA
- **GPIO 9**: I2C SCL
- **USB**: Connected to computer for Serial logging

### Setup B - Identical to Setup A
Replace:
- INA219 #1 → INA219 #2
- ESP32 #1 → ESP32 #3
- ESP32 #2 → ESP32 #4

---

## INA219 Specifications

### Measurement Ranges
- **Bus Voltage**: 0 to 26V
- **Shunt Voltage**: ±320mV (±40mV to ±320mV configurable)
- **Current**: ±3.2A (with 0.1Ω shunt resistor)
- **Power**: 0 to 83.2W

### Resolution
- **Current**: 0.8mA (with 0.1Ω shunt, 12-bit ADC)
- **Voltage**: 4mV
- **Power**: Calculated from V × I

### Accuracy
- **Shunt voltage**: ±0.5% (typical)
- **Bus voltage**: ±0.5% (typical)
- **Overall current**: ±1% with proper calibration

### I2C Interface
- **Default Address**: 0x40
- **Alternative Addresses**: 0x41, 0x44, 0x45 (via solder jumpers)
- **Clock Speed**: Up to 400kHz (Fast I2C)

### ESP32-C5 Typical Current Draw
- **Deep Sleep**: 5-20 µA (INA219 may not resolve accurately)
- **Light Sleep**: 0.5-2 mA
- **Idle (WiFi connected)**: 15-40 mA
- **WiFi TX**: 120-200 mA (peaks)
- **BLE Advertising**: 10-20 mA
- **BLE Connected**: 5-15 mA

**Note:** INA219 is excellent for mA-range measurements but may not accurately capture µA sleep currents. For deep sleep, consider using the Keysight multimeter.

---

## Calibration Procedure

### Step 1: Verify INA219 Modules

**Test both INA219 modules independently:**

1. Connect INA219 to ESP32 Logger
2. Connect known load (LED + resistor) through INA219
3. Calculate expected current: `I = V / R`
4. Compare INA219 reading to expected value
5. If error > 2%, check wiring or replace INA219

**Example:**
```
Load: Red LED (Vf = 2V) + 100Ω resistor
Applied voltage: 3.3V
Expected current: (3.3V - 2V) / 100Ω = 13mA
INA219 reading: 12.8mA → 1.5% error ✓ OK
```

### Step 2: ESP32 Baseline Comparison

**Verify both DUT ESP32s draw similar idle current:**

1. Upload blank sketch to both DUT-A and DUT-B:
   ```cpp
   void setup() { }
   void loop() { delay(1000); }
   ```
2. Measure idle current on both setups
3. Expected: 15-25 mA (no WiFi/BLE)
4. Difference should be < 2 mA

**If difference > 2mA:**
- Check for damaged ESP32
- Verify power supply voltage is identical
- Check for peripheral differences (LEDs, etc.)

### Step 3: Cross-Validation Test

**Run identical test on both setups:**

1. Upload `Idle_State_Power.ino` to DUT-A and DUT-B
2. Start logging on both Logger-A and Logger-B
3. Collect 60 seconds of data
4. Compare average current readings

**Acceptance Criteria:**
- Mean difference < 1 mA
- Standard deviation < 0.5 mA

**Example Results:**
```
Setup A: 23.4 mA ± 0.3 mA
Setup B: 23.6 mA ± 0.4 mA
Difference: 0.2 mA ✓ PASS
```

### Step 4: Keysight Verification (Optional)

**Use Keysight multimeter to verify one setup:**

1. Temporarily replace INA219 #1 with Keysight in current mode
2. Set bench power supply to 3.5V (compensate for burden voltage)
3. Run same idle test
4. Compare Keysight reading to INA219 #1 historical data

**Note:** Keysight should be within ±0.5 mA of INA219

---

## Test Protocol

### Phase 1: Calibration & Baseline (Day 1)

1. **Verify hardware**: Test both INA219 modules with known loads
2. **ESP32 comparison**: Confirm both DUTs draw similar idle current
3. **Cross-validation**: Run identical idle test on both setups
4. **Document baselines**: Record idle current for each ESP32

**Expected Time:** 1-2 hours

### Phase 2: Single Protocol Tests (Day 2-3)

Run each test on **both Setup A and Setup B** for cross-validation:

#### BLE Tests
1. `BLE_Advertisement_Power` (varying intervals)
2. `BLE_Connection_Lifecycle_Power`
3. `BLE_Data_Transfer_Power` (varying payload sizes)

#### WiFi Tests
1. `WiFi_Connection_Lifecycle_Power`
2. `WiFi_Data_Transfer_Power` (varying packet sizes)

#### Comparative Tests
1. `Idle_State_Power` (all states)
2. `Power_Crossover_Analysis`

**For each test:**
- Run on Setup A → collect data
- Run on Setup B → collect data
- Python script calculates average and std dev
- If std dev > 5%, investigate and re-run

**Expected Time:** 4-6 hours total

### Phase 3: Analysis & Validation (Day 4)

1. **Aggregate data**: Combine all measurements
2. **Calculate statistics**: Mean, std dev, confidence intervals
3. **Generate graphs**: Current vs payload size, energy per byte
4. **Identify crossover point**: Where WiFi becomes more efficient than BLE
5. **Validate outliers**: Re-test any suspicious data points

**Expected Time:** 2-3 hours

---

## Accuracy Guidelines

### Measurement Accuracy by Configuration

| Configuration | Idle Current | Active Current | Sleep Current | Overall |
|--------------|--------------|----------------|---------------|---------|
| Self-Measurement (Option 1) | ±5 mA | ±10 mA | Cannot measure | ~85% |
| Dedicated Logger (Option 2) | ±1 mA | ±2 mA | ±0.1 mA | ~95% |
| **Dual Independent (Option 3)** | **±0.5 mA** | **±1 mA** | **±0.05 mA** | **~99%** |
| Differential (Option 4) | ±0.3 mA | ±0.5 mA | ±0.02 mA | ~99.5% |

### Sources of Measurement Error

#### Systematic Errors (can be calibrated out)
- **INA219 offset error**: ±0.5% typical
- **Shunt resistor tolerance**: ±1% (0.1Ω ± 0.001Ω)
- **Power supply voltage variation**: ±0.1V affects current slightly
- **Temperature drift**: ~0.01%/°C

#### Random Errors (reduced by averaging)
- **ADC noise**: ±0.8 mA (INA219 resolution limit)
- **ESP32 current variability**: ±1-2 mA (RF sensitivity to environment)
- **Timing jitter**: Serial logging delays can shift data points

#### How to Minimize Errors
1. ✅ **Use dual setup**: Averages out random errors
2. ✅ **Multiple runs**: Run each test 3-5 times, average results
3. ✅ **Controlled environment**: Minimize WiFi interference, keep temperature stable
4. ✅ **Calibration**: Verify with known loads before each test session
5. ✅ **Synchronization**: Timestamp all measurements accurately

### Statistical Confidence

**With dual independent setup (Option 3):**

| Test Runs | Confidence Interval | Use Case |
|-----------|---------------------|----------|
| 1 run per setup (2 total) | ±2 mA (95%) | Quick screening |
| 3 runs per setup (6 total) | ±1 mA (95%) | Standard testing |
| 5 runs per setup (10 total) | ±0.5 mA (99%) | Publication-quality |

**Formula for uncertainty:**
```
Uncertainty = (Std Dev / √N) × t-factor
Where N = number of measurements
      t-factor = 1.96 for 95% confidence
```

---

## Safety Considerations

### Electrical Safety

#### Power Supply Settings
- ✅ **Voltage**: Set to 3.3V ±0.1V (ESP32-C5 absolute max: 3.6V)
- ✅ **Current limit**: Set to 500 mA (protects against shorts)
- ⚠️ **Never exceed 3.6V** on ESP32 VCC - permanent damage will occur

#### INA219 Wiring
- ✅ **Double-check polarity**: VIN+ to PSU, VIN- to ESP32
- ⚠️ **Reverse polarity can damage INA219** (no reverse protection)
- ✅ **Shunt resistor rating**: 0.1Ω 1W (do not exceed 3.2A)

#### Common Ground
- ✅ **All grounds must be connected**: PSU, INA219, both ESP32s
- ⚠️ **Floating ground causes erratic readings**

#### USB Power vs Bench Supply
- ✅ **DUT**: Always use bench supply through INA219 (for measurement)
- ✅ **Logger**: Can use USB power (isolated from DUT)
- ⚠️ **Never power DUT from USB and bench supply simultaneously** (ground loops)

### Handling Precautions

#### ESD Protection
- ✅ Use anti-static wrist strap when handling ESP32s
- ✅ Work on anti-static mat if available
- ⚠️ ESP32-C5 is sensitive to static discharge

#### Physical Setup
- ✅ Keep wiring neat to avoid accidental disconnections
- ✅ Use breadboards or terminal blocks for secure connections
- ✅ Label each ESP32 (DUT-A, Logger-A, etc.) to avoid confusion

#### During Measurements
- ⚠️ **Do not disconnect INA219 while powered** (can cause voltage spikes)
- ⚠️ **Do not touch ESP32 antenna** during RF tests (affects readings)
- ✅ Keep setup away from metal objects (affects RF propagation)

---

## Troubleshooting

### Problem: INA219 reads 0.0 mA

**Possible Causes:**
1. I2C communication failure
2. Wrong I2C address
3. INA219 not powered (VCC/GND)

**Solutions:**
- Run I2C scanner sketch, verify address (should be 0x40)
- Check SDA/SCL connections (try swapping)
- Verify INA219 VCC = 3.3V with multimeter
- Check INA219 library installed correctly

### Problem: Current reading is negative

**Possible Causes:**
1. VIN+ and VIN- reversed
2. Current flowing backward through shunt

**Solutions:**
- Swap VIN+ and VIN- connections
- Verify power supply is connected to VIN+, ESP32 to VIN-

### Problem: Readings unstable/noisy (±10 mA swings)

**Possible Causes:**
1. Poor ground connection
2. Power supply noise
3. WiFi interference
4. Loose wiring

**Solutions:**
- Check all ground connections are solid
- Add 10µF capacitor across ESP32 VCC/GND (close to chip)
- Move setup away from WiFi router
- Re-seat all jumper wires

### Problem: Setup A and Setup B differ by > 5 mA

**Possible Causes:**
1. Different ESP32 chips (manufacturing variation)
2. One INA219 is miscalibrated
3. Different power supply voltages
4. One ESP32 has damaged RF section

**Solutions:**
- Measure power supply voltage on both setups (should be ±0.05V)
- Swap INA219 modules to isolate sensor vs ESP32 issue
- Test both ESP32s with LED load (should draw same current)
- Recalibrate INA219 with known load

### Problem: Deep sleep current shows as 0 mA

**Possible Causes:**
1. INA219 resolution limit (~0.8 mA)
2. Deep sleep current too low to measure

**Solutions:**
- This is expected - INA219 cannot resolve µA currents
- Use Keysight multimeter (µA range) for sleep measurements
- Or use specialized low-current meter (e.g., µCurrent Gold)

### Problem: WiFi test shows intermittent high current spikes

**Possible Causes:**
1. This is normal - WiFi TX bursts can reach 200+ mA
2. INA219 sampling rate (1ms) may miss some spikes

**Solutions:**
- This is expected behavior
- Use averaging in analysis (mean over 1 second windows)
- If need to capture spikes: increase INA219 sampling rate or use oscilloscope

### Problem: Cannot upload code to DUT ESP32

**Possible Causes:**
1. Power supply current-limited during boot
2. Insufficient voltage through INA219 (burden voltage)

**Solutions:**
- Temporarily bypass INA219 (direct USB power) for programming
- Or increase power supply voltage to 3.4V during upload
- After upload, restore to 3.3V for measurement

---

## Data Logging Best Practices

### Timestamp Synchronization
- Use NTP or manual sync to align Logger-A and Logger-B timestamps
- Include millisecond precision: `YYYY-MM-DD HH:MM:SS.mmm`

### File Naming Convention
```
<Test_Name>_<Setup>_<Run>_<Date>.csv

Examples:
BLE_Data_Transfer_SetupA_Run1_2025-11-02.csv
WiFi_Idle_SetupB_Run2_2025-11-02.csv
```

### CSV Format
```csv
timestamp,voltage_V,current_mA,power_mW,test_phase
2025-11-02 14:30:15.123,3.28,23.4,76.8,idle
2025-11-02 14:30:16.124,3.27,45.2,147.8,wifi_connect
```

### Metadata to Record
- ESP32 chip ID (MAC address)
- INA219 module ID (label them #1, #2)
- Power supply voltage setting
- Room temperature
- WiFi SSID and signal strength (for WiFi tests)
- BLE advertising interval (for BLE tests)

---

## References

### Datasheets
- [INA219 Datasheet (Texas Instruments)](https://www.ti.com/lit/ds/symlink/ina219.pdf)
- [ESP32-C5 Datasheet (Espressif)](https://www.espressif.com/sites/default/files/documentation/esp32-c5_datasheet_en.pdf)

### Libraries
- [Adafruit INA219 Arduino Library](https://github.com/adafruit/Adafruit_INA219)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)

### Application Notes
- [INA219 Application Guide](https://www.ti.com/lit/an/sbaa105a/sbaa105a.pdf)
- [ESP32 Current Consumption Measurement](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c5/api-guides/current-consumption-measurement-modules.html)

---

## Appendix: Quick Reference

### I2C Pin Mapping (ESP32-C5)

| Function | GPIO Pin | Alternative |
|----------|----------|-------------|
| SDA | GPIO 8 | GPIO 6 |
| SCL | GPIO 9 | GPIO 7 |

### Power Supply Quick Check

```
✓ Voltage: 3.3V ± 0.1V
✓ Current limit: 500 mA
✓ Ripple: < 50 mV
✓ All grounds connected
✓ INA219 VCC = 3.3V
```

### Pre-Test Checklist

- [ ] Both INA219 modules tested with LED load
- [ ] Both ESP32 DUTs show similar idle current
- [ ] Logger ESP32s communicate with INA219 via I2C
- [ ] Serial connections to computer working
- [ ] Power supply voltage verified with multimeter
- [ ] All wiring double-checked (polarity, connections)
- [ ] Python logging script ready
- [ ] Test scripts uploaded to DUTs

---

**Document Version:** 1.0
**Last Updated:** 2025-11-02
**Author:** XiFi Project Team
**Course:** CS 439 - UIUC Fall 2025
