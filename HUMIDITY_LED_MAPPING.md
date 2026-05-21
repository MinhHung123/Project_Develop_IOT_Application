# Humidity-Based NeoPixel LED Color Mapping

## Overview
This implementation provides real-time humidity monitoring with visual feedback through NeoPixel RGB LED color changes. The system uses **FreeRTOS semaphore synchronization** to safely share humidity data between the temperature/humidity sensor task and the LED display task.

---

## 📊 Humidity Level to Color Mapping

| Humidity Range | Level Name | RGB Color | Hex Code | Meaning |
|---|---|---|---|---|
| **0% - 30%** | DRY | Blue `(0, 0, 255)` | `#0000FF` | Air is dry, low moisture |
| **30% - 60%** | NORMAL | Green `(0, 255, 0)` | `#00FF00` | Comfortable/optimal humidity |
| **60% - 100%** | HUMID | Red `(255, 0, 0)` | `#FF0000` | High humidity, moist air |

---

## 🔧 Technical Implementation

### 1. **Semaphore Synchronization Mechanism**

#### Binary Semaphore: `xSemaphoreLEDUpdate`

The system uses a **binary semaphore** to protect shared data (humidity value) between two FreeRTOS tasks:

- **Temperature/Humidity Task** → Reads sensor data and updates `glob_humidity`
- **LED Task** → Reads `glob_humidity` and updates NeoPixel color

```
┌─────────────────────────────────────────────────────────┐
│          FreeRTOS Task Synchronization Flow             │
├─────────────────────────────────────────────────────────┤
│                                                         │
│ Temp/Humid Task          LED Task                       │
│       │                      │                          │
│  [Read Sensor]               │                          │
│       │                      │                          │
│  xSemaphoreTake()            │                          │
│       │                      │                          │
│  [Update glob_humidity]      │                          │
│       │                      │                          │
│  xSemaphoreGive()        xSemaphoreTake()               │
│       │                      │                          │
│  [Wait 2s]             [Read humidity]                  │
│       │                      │                          │
│       │                [Calculate color]                │
│       │                      │                          │
│       │                 xSemaphoreGive()                │
│       │                      │                          │
│       │                [Update NeoPixel]                │
│       │                      │                          │
│       │                 [Wait 500ms]                    │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 2. **Color Determination Logic**

The `getColorByHumidity()` function maps humidity values to LED colors:

```c
if (humidity <= 30%) {
    color = BLUE (0, 0, 255)      // Level 1: DRY
}
else if (humidity <= 60%) {
    color = GREEN (0, 255, 0)     // Level 2: NORMAL
}
else {
    color = RED (255, 0, 0)       // Level 3: HUMID
}
```

### 3. **Semaphore Configuration**

In `global.cpp`:
```cpp
SemaphoreHandle_t xSemaphoreLEDUpdate = xSemaphoreCreateBinary();
```

In `main.cpp`, the semaphore is initialized:
```cpp
xSemaphoreGive(xSemaphoreLEDUpdate);  // Give initial permission
```

---

## 📝 Code Changes Summary

### Modified Files:

#### **1. `include/global.h`**
- Added semaphore declaration: `xSemaphoreLEDUpdate`
- Added humidity-color mapping documentation
- Added `HumidityColorMap` structure

#### **2. `src/global.cpp`**
- Created binary semaphore: `xSemaphoreLEDUpdate`

#### **3. `include/task_neo_led.h`**
- Added humidity level thresholds:
  - `HUMID_LEVEL1_MAX = 30`
  - `HUMID_LEVEL2_MAX = 60`
- Added color macros for each level

#### **4. `src/task_neo_led.cpp`**
- Implemented `getColorByHumidity()` function
- Added semaphore-protected humidity reading
- Added intelligent update logic (only updates on significant change ≥2%)
- Added detailed serial logging for debugging

#### **5. `src/task_temp_humid.cpp`**
- Wrapped global humidity updates with semaphore protection
- Added proper error handling
- Added task identification in serial output

#### **6. `src/main.cpp`**
- Added semaphore initialization verification
- Added initial semaphore grant

---

## ⏱️ Task Timing

- **Temperature/Humidity Task**: Reads sensor every **2000ms**
- **LED Task**: Updates LED color every **500ms** (with significant change detection)
- **Semaphore Timeout**: **1000ms** per task

### Example Timeline:
```
T=0ms     : Temp/Humid reads sensor, acquires semaphore
T=10ms    : Temp/Humid releases semaphore with new humidity
T=500ms   : LED task acquires semaphore, reads humidity, updates color
T=1000ms  : LED updates pixel
T=1500ms  : LED updates pixel again
T=2000ms  : Temp/Humid reads sensor again
T=2500ms  : LED task checks for new humidity changes
```

---

## 🎯 Feature Highlights

✅ **3 Humidity Levels** with distinct colors  
✅ **Binary Semaphore Synchronization** for thread-safe data sharing  
✅ **Smart Update Detection** - only updates LED when humidity changes significantly  
✅ **Error Handling** - displays white LED on sensor error  
✅ **Detailed Logging** - clear serial output showing humidity levels and colors  
✅ **Non-blocking Synchronization** - tasks can timeout gracefully  

---

## 🔍 Serial Output Example

```
=== NeoPixel LED Task Started ===
Humidity -> LED Color Mapping:
  0-30% (DRY)   -> BLUE   (RGB: 0, 0, 255)
  30-60% (NORM) -> GREEN  (RGB: 0, 255, 0)
  60-100% (HUM) -> RED    (RGB: 255, 0, 0)
==================================

[MAIN] LED synchronization semaphore initialized
=== Temperature & Humidity Task Started ===

[SENSOR] Temperature: 25.3 °C, Humidity: 45.2 %
[LED] Humidity Level: NORMAL (45.2 %) -> Green (RGB: 0, 255, 0)

[SENSOR] Temperature: 25.5 °C, Humidity: 72.1 %
[LED] Humidity Level: HUMID (72.1 %) -> Red (RGB: 255, 0, 0)

[SENSOR] Temperature: 24.8 °C, Humidity: 18.5 %
[LED] Humidity Level: DRY (18.5 %) -> Blue (RGB: 0, 0, 255)
```

---

## 🛡️ Synchronization Benefits

1. **Data Consistency**: Ensures `glob_humidity` is completely written before LED reads it
2. **Race Condition Prevention**: No simultaneous read/write of shared variable
3. **Task Coordination**: LED task waits for valid sensor data before update
4. **Graceful Degradation**: Timeout prevents task deadlock

---

## 🚀 Extending the System

To add more humidity levels, modify in `include/task_neo_led.h`:

```c
#define HUMID_LEVEL3_MAX 75     // Add 4th level
#define HUMID_LEVEL4_MAX 90     // Add 5th level
```

And update `getColorByHumidity()` in `src/task_neo_led.cpp` to handle new ranges.

---

## 📚 Resources

- **FreeRTOS Semaphore Documentation**: https://www.freertos.org/xSemaphoreTake.html
- **Adafruit NeoPixel Library**: https://github.com/adafruit/Adafruit_NeoPixel
- **DHT20 Sensor Datasheet**: Temperature and Humidity Sensor

