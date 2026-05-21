# Humidity Level to NeoPixel Color Reference Guide

## 📊 Color Mapping Chart

```
HUMIDITY LEVEL           COLOR          RGB VALUE      HEX       INDICATOR
─────────────────────────────────────────────────────────────────────────

│███████████████│  0% 
│               │  
│               │  5%
│               │  
│               │  10%
│               │  BLUE             (0, 0, 255)    #0000FF    ↓ DRY
│               │  15%
│               │  
│               │  20%
│               │  
│               │  25%
│               │  
│               │  30%  ─────────────────────────────────────────────
│███████████████│       
│               │  35%
│               │  
│               │  40%  GREEN           (0, 255, 0)   #00FF00   ↓ NORMAL
│               │  45%
│               │  
│               │  50%
│               │  
│               │  55%
│               │  
│               │  60%  ─────────────────────────────────────────────
│████████████████
│                │  65%
│                │  
│                │  70%  RED            (255, 0, 0)   #FF0000   ↓ HUMID
│                │  75%
│                │  
│                │  80%
│                │  
│                │  85%
│                │  
│                │  90%
│                │  
│                │  95%
│                │  
│                │  100%

```

## 🎨 RGB Color Values

### Level 1: DRY (0-30%)
- **Color Name**: Blue
- **RGB**: (R=0, G=0, B=255)
- **Hex**: #0000FF
- **Decimal**: 255
- **Meaning**: Low moisture level, air is dry

### Level 2: NORMAL (30-60%)
- **Color Name**: Green
- **RGB**: (R=0, G=255, B=0)
- **Hex**: #00FF00
- **Decimal**: 65280
- **Meaning**: Comfortable humidity range

### Level 3: HUMID (60-100%)
- **Color Name**: Red
- **RGB**: (R=255, G=0, B=0)
- **Hex**: #FF0000
- **Decimal**: 16711680
- **Meaning**: High humidity, moist environment

## 📋 Comparison Table

| Feature | Level 1 | Level 2 | Level 3 |
|---------|---------|---------|---------|
| Humidity Range | 0-30% | 30-60% | 60-100% |
| Level Name | DRY | NORMAL | HUMID |
| Color Name | Blue | Green | Red |
| RGB Values | (0, 0, 255) | (0, 255, 0) | (255, 0, 0) |
| Hex Code | #0000FF | #00FF00 | #FF0000 |
| Brightness | Lowest | Medium | Highest |
| User Action | Add moisture | - | Remove moisture |

## 🔄 State Transition Logic

```
        Humidity Reading
                ↓
         ┌─────────────┐
         │  Is < 30%?  │
         └──────┬──────┘
                │
        YES ─────┴─────── NO
        │                 │
        ↓                 ↓
    Display BLUE      ┌─────────────┐
    RGB(0,0,255)      │  Is < 60%?  │
                      └──────┬──────┘
                             │
                     YES ─────┴─────── NO
                     │                 │
                     ↓                 ↓
                 Display GREEN     Display RED
                RGB(0,255,0)      RGB(255,0,0)
```

## ⚙️ Implementation in Code

### Definition in task_neo_led.h
```cpp
#define HUMID_LEVEL1_MAX 30      // 0-30% = Dry (Blue)
#define HUMID_LEVEL2_MAX 60      // 30-60% = Normal (Green)
                                 // 60-100% = Humid (Red)

#define COLOR_DRY_BLUE     strip.Color(0, 0, 255)
#define COLOR_NORMAL_GREEN strip.Color(0, 255, 0)
#define COLOR_HUMID_RED    strip.Color(255, 0, 0)
```

### Function Logic in task_neo_led.cpp
```cpp
void getColorByHumidity(float humidity, uint32_t &color, 
                       const char* &level_name, 
                       Adafruit_NeoPixel &strip) {
    if (humidity <= HUMID_LEVEL1_MAX) {
        // 0-30% → BLUE
        color = COLOR_DRY_BLUE;
        level_name = "DRY";
    }
    else if (humidity <= HUMID_LEVEL2_MAX) {
        // 30-60% → GREEN
        color = COLOR_NORMAL_GREEN;
        level_name = "NORMAL";
    }
    else {
        // 60-100% → RED
        color = COLOR_HUMID_RED;
        level_name = "HUMID";
    }
}
```

## 📡 Synchronization Timeline

```
SYNC POINT 1: Humidity Sensor Task
─────────────────────────────────
Time: 0ms
┌─────────────────────────────────┐
│ TempHumid Task                  │
│ - Read DHT20 sensor             │
│ - Get humidity value            │
│ - xSemaphoreTake()              │ ← ACQUIRE LOCK
│ - Update glob_humidity          │
│ - xSemaphoreGive()              │ ← RELEASE LOCK
│ - Wait 2000ms until next read   │
└─────────────────────────────────┘
         ↓
         │ Humidity Updated
         ↓

SYNC POINT 2: LED Color Update Task
────────────────────────────────────
Time: 500ms (500ms after humidity read)
┌─────────────────────────────────┐
│ LED Task                        │
│ - xSemaphoreTake()              │ ← WAIT FOR LOCK OR TIMEOUT
│ - Read glob_humidity            │
│ - Calculate new color           │
│ - xSemaphoreGive()              │ ← RELEASE LOCK
│ - Update strip.setPixelColor()  │
│ - Display color on NeoPixel     │
│ - Wait 500ms for next update    │
└─────────────────────────────────┘
```

## 🧪 Testing the Implementation

### Test Case 1: DRY Humidity
- Set sensor to read ~20% humidity
- **Expected**: NeoPixel should display **BLUE** (0, 0, 255)
- **Serial Output**: `[LED] Humidity Level: DRY (20.0 %) -> Blue (RGB: 0, 0, 255)`

### Test Case 2: NORMAL Humidity
- Set sensor to read ~45% humidity
- **Expected**: NeoPixel should display **GREEN** (0, 255, 0)
- **Serial Output**: `[LED] Humidity Level: NORMAL (45.0 %) -> Green (RGB: 0, 255, 0)`

### Test Case 3: HUMID Humidity
- Set sensor to read ~75% humidity
- **Expected**: NeoPixel should display **RED** (255, 0, 0)
- **Serial Output**: `[LED] Humidity Level: HUMID (75.0 %) -> Red (RGB: 255, 0, 0)`

### Test Case 4: Sensor Error
- Disconnect DHT20 sensor or force error
- **Expected**: NeoPixel should display **WHITE** (255, 255, 255)
- **Serial Output**: `[LED] Humidity sensor error - displaying white`

