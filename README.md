# YoloUNO IoT Platform Project - Hệ Thống Nhà Thông Minh

Dự án IoT hoàn chỉnh sử dụng **ESP32-S3 (YoloUNO)** để kết nối với **CoreIOT** qua MQTT, điều khiển LED, đọc cảm biến nhiệt độ/độ ẩm, và nhận lệnh RPC từ server.

---

## 💡 Ý Tưởng Nhà Thông Minh (Smart Home Concept)

### 🎯 Tầm Nhìn Chung

Dự án này xây dựng một **hệ thống nhà thông minh tích hợp** cho phép:

- **Giám sát môi trường**: Đọc nhiệt độ và độ ẩm từ cảm biến DHT20 theo thời gian thực
- **Điều khiển thiết bị từ xa**: Quản lý quạt, cửa, và đèn thông qua Cloud (CoreIOT)
- **Tự động hóa thông minh**: Thiết bị tự động phản ứng dựa trên các điều kiện môi trường
- **Giám sát trực tiếp**: LED NeoPixel hiển thị trạng thái độ ẩm theo màu sắc

### 🏠 Kịch Bản Sử Dụng

#### Scenario 1: Điều Khiển Độ Ẩm Tự Động
```
Người dùng muốn duy trì độ ẩm phòng ở mức lý tưởng (30-60%)
    ↓
Cảm biến DHT20 đọc độ ẩm mỗi 2 giây
    ↓
Nếu độ ẩm > 60% → LED bật màu ĐỎ + Quạt bật để làm khô phòng
Nếu độ ẩm 30-60% → LED bật màu XANH LỤC (Tối ưu)
Nếu độ ẩm < 30% → LED bật màu XANH (Quá khô)
```

#### Scenario 2: Điều Khiển Từ Xa Qua Cloud
```
Người dùng → CoreIOT Dashboard → MQTT RPC Command
    ↓
ESP32 nhận lệnh (setFan, setDoor, setColor)
    ↓
Thực thi lệnh → Quạt, Cửa, LED thay đổi
    ↓
Gửi lại trạng thái hiện tại cho Cloud
```

#### Scenario 3: Cảnh Báo Thông Minh
```
Nếu phát hiện bất thường:
- Độ ẩm quá cao (>80%) → LED nhấp nháy ĐỎ
- Nhiệt độ quá cao (>35°C) → Gửi cảnh báo cho Cloud
- Quạt không hoạt động → Tự động thử khởi động lại
```

### 🔄 Luồng Dữ Liệu Hệ Thống

```
┌─────────────────────────────────────────────────────────────┐
│                    CoreIOT Cloud Server                      │
│          (MQTT Broker - app.coreiot.io:1883)                │
└────────┬─────────────────────────────────────┬──────────────┘
         │                                      │
         │ RPC Commands                         │ Telemetry Data
         │ (setFan, setDoor, setColor)          │ (temp, humidity, fan_speed)
         │                                      │
    ┌────▼──────────────────────────────────────▼────┐
    │           ESP32-S3 (YoloUNO Device)            │
    │                                                 │
    │  ┌──────────────┐  ┌──────────────┐           │
    │  │  DHT20       │  │  NeoPixel    │           │
    │  │  Sensor      │  │  LED         │           │
    │  │  (I2C)       │  │  (GPIO48)    │           │
    │  └──────────────┘  └──────────────┘           │
    │                                                 │
    │  ┌──────────────┐  ┌──────────────┐           │
    │  │  PWM Fan     │  │  Servo Door  │           │
    │  │  (GPIO10)    │  │  (GPIO5)     │           │
    │  └──────────────┘  └──────────────┘           │
    │                                                 │
    │  ┌──────────────────────────────────┐         │
    │  │   FreeRTOS Task Manager          │         │
    │  │  - TempHumidTask (Priority 1)    │         │
    │  │  - LED Control (Priority 2)      │         │
    │  │  - CoreIOT Task (Priority 3)     │         │
    │  └──────────────────────────────────┘         │
    └─────────────────────────────────────────────────┘
```

### 🎨 Trạng Thái Màu LED (Color Mapping)

| Độ Ẩm (%) | Màu | Ý Nghĩa | Hành Động |
|-----------|-----|--------|----------|
| 0-30% | 🔵 BLUE | DRY - Quá khô | Có thể bật máy tăm ẩm |
| 30-60% | 🟢 GREEN | NORMAL - Tối ưu | Duy trì trạng thái hiện tại |
| 60-100% | 🔴 RED | HUMID - Quá ẩm | Bật quạt để làm khô |
| ERROR | ⚪ WHITE | Lỗi cảm biến | Kiểm tra kết nối cảm biến |

## 📋 Tính Năng Chính

✅ **Kết nối WiFi Station Mode** - Tự động kết nối đến WiFi network  
✅ **Cảm biến DHT20** - Đọc nhiệt độ và độ ẩm qua I2C  
✅ **Điều khiển LED NeoPixel** - Hiển thị trạng thái độ ẩm theo màu sắc  
✅ **MQTT CoreIOT Integration** - Gửi/nhận dữ liệu với cloud  
✅ **RPC Command Processing** - Nhận và xử lý lệnh từ server (quạt, cửa, đèn)  
✅ **Multi-tasking với FreeRTOS** - 4 tasks chạy song song  
✅ **Semaphore Synchronization** - Bảo vệ dữ liệu chia sẻ giữa các tasks  

## 🔧 Phần Cứng

- **Board**: YoloUNO (ESP32-S3)
- **Cảm biến**: DHT20 (I2C - SDA: GPIO11, SCL: GPIO12)
- **Đầu ra**: 
  - LED NeoPixel (GPIO48)
  - PWM Fan (GPIO10) - Quạt thông gió
  - Servo Motor (GPIO5) - Cửa tự động
- **Kết nối**: WiFi 2.4GHz, MQTT

## 📦 Thư Viện Sử Dụng

```
- ArduinoJson (JSON parsing)
- PubSubClient@2.8.0 (MQTT client)
- Thingsboard@0.13.0 (CoreIOT integration)
- ArduinoHttpClient (HTTP client)
- DHT20 (Temperature/Humidity sensor)
- Adafruit NeoPixel @ ^1.15.4 (LED control)
- ESP32Servo @ ^1.1.8 (Servo motor control)
```

## 🚀 Quy Trình Khởi Động

1. **Serial Communication** - 115200 baud
2. **WiFi Setup** - Kết nối với SSID/Password từ global.cpp
3. **I2C Initialization** - Chuẩn bị đọc DHT20
4. **FreeRTOS Tasks khởi động**:
   - `TempHumidTask` (Priority 1) - Đọc cảm biến mỗi 2 giây
   - `LED Control Task` (Priority 2) - Cập nhật LED mỗi 500ms
   - `CoreIOT Task` (Priority 3) - Kết nối MQTT & xử lý RPC

---

## 🔗 Rule Chain - Giao Tiếp Giữa 2 Thiết Bị (Device Communication)

### 📌 Khái Niệm Rule Chain

**Rule Chain** là một hệ thống quy tắc tự động cho phép 2 hoặc nhiều thiết bị giao tiếp với nhau thông qua Cloud (CoreIOT), tạo ra các hành động liên động dựa trên điều kiện đã định.

Thay vì mỗi thiết bị hoạt động độc lập, Rule Chain cho phép:
- **Device A** gửi dữ liệu lên Cloud (ví dụ: độ ẩm)
- **Cloud** xử lý quy tắc và quyết định
- **Device B** nhận lệnh từ Cloud để thực thi (ví dụ: bật đèn)

### 🎯 Ví Dụ Thực Tế: Humidity-Triggered LED

#### Kịch Bản
```
Một nhà có 2 thiết bị YoloUNO:
- Thiết bị 1 (Phòng khách): Chứa cảm biến DHT20 để đọc độ ẩm
- Thiết bị 2 (Phòng ngủ): Chứa LED để cảnh báo
```

#### Luồng Giao Tiếp

```
TIME: 00:00:00

┌──────────────────────────────┐
│    Thiết Bị 1: Phòng Khách   │
│         (YoloUNO #1)         │
└────────────┬─────────────────┘
             │
             │ 1. Đọc cảm biến DHT20
             │    Humidity = 65%
             │
             ▼
    ┌────────────────────┐
    │   Publish MQTT     │
    │ Topic: telemetry   │
    │ Payload:           │
    │ {humidity: 65}     │
    └────────┬───────────┘
             │
             │ 2. Gửi lên CoreIOT Cloud
             │    via MQTT:1883
             │
             ▼
    ┌──────────────────────────────────┐
    │      CoreIOT Cloud Server        │
    │    (Rule Engine Processing)      │
    │                                  │
    │  Rule: IF humidity > 60% THEN    │
    │        SET device2.led = ON      │
    │        SET device2.color = RED   │
    └────────┬─────────────────────────┘
             │
             │ 3. Gửi RPC Command
             │    Tới Thiết Bị 2
             │
             ▼
┌──────────────────────────────┐
│   Thiết Bị 2: Phòng Ngủ      │
│        (YoloUNO #2)          │
│                              │
│ 4. Nhận RPC Command:         │
│    {method: "setColor",      │
│     params: {r:255, g:0, b:0}} │
│                              │
│ 5. Thực thi hành động:       │
│    - Bật LED màu ĐỎ         │
│    - In log: "LED ON - RED"  │
└──────────────────────────────┘
```

### 📋 Chi Tiết Quy Tắc (Rule Details)

#### Rule 1: Humidity High Detection
| Thành Phần | Giá Trị |
|-----------|--------|
| **Trigger** | Độ ẩm từ Device 1 > 60% |
| **Condition** | `humidity >= 60 AND humidity <= 100` |
| **Action** | Gửi lệnh tới Device 2 |
| **Command** | `setColor(255, 0, 0)` - Màu ĐỎ |
| **Delay** | 0 (ngay lập tức) |

#### Rule 2: Humidity Normal Range
| Thành Phần | Giá Trị |
|-----------|--------|
| **Trigger** | Độ ẩm từ Device 1 = 30-60% |
| **Condition** | `humidity > 30 AND humidity < 60` |
| **Action** | Gửi lệnh tới Device 2 |
| **Command** | `setColor(0, 255, 0)` - Màu XANH LỤC |
| **Delay** | 0 |

#### Rule 3: Humidity Low Alert
| Thành Phần | Giá Trị |
|-----------|--------|
| **Trigger** | Độ ẩm từ Device 1 < 30% |
| **Condition** | `humidity < 30` |
| **Action** | Gửi lệnh tới Device 2 |
| **Command** | `setColor(0, 0, 255)` - Màu XANH |
| **Delay** | 0 |

### 🔄 Quá Trình Triển Khai Trong Code

#### Thiết Bị 1: Gửi Dữ Liệu (Phòng Khách)

```cpp
// File: src/task_temp_humid.cpp
// Mỗi 2 giây đọc cảm biến và publish lên MQTT

void TaskTempHumid(void *pvParameters) {
    while(1) {
        // Đọc DHT20
        float temperature = dht20.getTemperature();
        float humidity = dht20.getHumidity();
        
        // Cập nhật global variable
        xSemaphoreTake(xBinarySemaphoreTemp_Humi, pdMS_TO_TICKS(500));
        glob_temperature = temperature;
        glob_humidity = humidity;
        xSemaphoreGive(xBinarySemaphoreTemp_Humi);
        
        // Publish lên Cloud
        publishSensor(temperature, humidity, fan_speed);
        
        Serial.printf("Humidity: %.2f%% -> Sending to Cloud\n", humidity);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

**Output Console:**
```
Humidity: 65.30% -> Sending to Cloud
Published: {"humidity":65.30,"temperature":22.50,"fan_speed":50}
```

#### Cloud Processing (CoreIOT Rule Engine)

CoreIOT nhận dữ liệu từ Device 1 qua MQTT topic:
```
Topic: v1/devices/me/telemetry
Payload: {"humidity": 65.30}
```

**Rule Engine xử lý:**
```
IF topic == "v1/devices/me/telemetry" 
AND payload.humidity > 60
THEN:
  1. Gửi RPC command tới device2
  2. Method: "setColor"
  3. Params: {r: 255, g: 0, b: 0}
```

#### Thiết Bị 2: Nhận Lệnh & Thực Thi (Phòng Ngủ)

```cpp
// File: src/task_coreIOT.cpp
// Hàm callback MQTT nhận RPC commands

void callback(char* topic, byte* payload, unsigned int length) {
    // Parse MQTT message
    JsonDocument doc;
    deserializeJson(doc, payload);
    
    String method = doc["method"];
    
    // Xử lý lệnh setColor từ Rule Chain
    if (method == "setColor") {
        uint8_t r = doc["params"]["r"];
        uint8_t g = doc["params"]["g"];
        uint8_t b = doc["params"]["b"];
        
        // Đặt LED thành màu chỉ định
        setNeoPixelColor(r, g, b);
        
        Serial.printf("LED ON - Color: R:%d G:%d B:%d\n", r, g, b);
    }
}

// Hàm đặt màu LED NeoPixel
void setNeoPixelColor(uint8_t r, uint8_t g, uint8_t b) {
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
}
```

**Output Console (Device 2):**
```
Message arrived [v1/devices/me/rpc/request/1]
{"method":"setColor","params":{"r":255,"g":0,"b":0}}
LED ON - Color: R:255 G:0 B:0
```

### 🎨 Bảng Quy Tắc Hoàn Chỉnh

```
Độ Ẩm Device 1    │   Trigger Rule   │   RPC Command (Device 2)  │  Kết Quả LED
─────────────────┼──────────────────┼───────────────────────────┼────────────────
   < 30%         │  Quá khô         │  setColor(0, 0, 255)      │  LED XANH
   30-60%        │  Tối ưu          │  setColor(0, 255, 0)      │  LED XANH LỤC
   > 60%         │  Quá ẩm          │  setColor(255, 0, 0)      │  LED ĐỎ
   ERROR/NaN     │  Lỗi cảm biến    │  setColor(255, 255, 255)  │  LED TRẮNG (Warning)
```

### ⚙️ Cách Thiết Lập Rule Chain Trên CoreIOT

1. **Đăng nhập CoreIOT Dashboard** → https://app.coreiot.io
2. **Tạo 2 Devices:**
   - Device 1: "Phong_Khach_Sensor" (với cảm biến DHT20)
   - Device 2: "Phong_Ngu_LED" (với LED NeoPixel)

3. **Vào mục "Rule Chains"** → Create New Rule Chain

4. **Thêm Rule:**
   ```
   Input: Message from Device 1 telemetry
   Filter: IF humidity > 60
   Action: Send RPC to Device 2 with setColor(255,0,0)
   ```

5. **Deploy Rule Chain** → Bật trạng thái Active

6. **Test:** 
   - Thay đổi độ ẩm ở Device 1
   - Quan sát LED ở Device 2 thay đổi theo

### 📊 Mô Tả Luồng Dữ Liệu Đầy Đủ

```
Device 1 Sensor Reading
        │
        ▼ (Every 2 seconds)
  ┌──────────────┐
  │ DHT20.read() │
  │ humidity=65% │
  └──────┬───────┘
         │
         ▼ (WiFi MQTT)
  ┌──────────────────────────┐
  │ MQTT Publish             │
  │ Topic: telemetry         │
  │ {"humidity": 65}         │
  └──────┬───────────────────┘
         │
         ▼ (Internet)
  ┌──────────────────────────┐
  │ CoreIOT MQTT Broker      │
  │ (app.coreiot.io:1883)    │
  └──────┬───────────────────┘
         │
         ▼ (Rule Engine)
  ┌──────────────────────────┐
  │ Rule Processing:         │
  │ IF humidity > 60% THEN   │
  │   RPC → Device 2         │
  └──────┬───────────────────┘
         │
         ▼ (RPC Command)
  ┌──────────────────────────┐
  │ Send RPC:                │
  │ setColor(255, 0, 0)      │
  └──────┬───────────────────┘
         │
         ▼ (Internet)
  ┌──────────────────────────┐
  │ Device 2 Receive RPC     │
  │ callback() triggered     │
  └──────┬───────────────────┘
         │
         ▼ (GPIO Output)
  ┌──────────────────────────┐
  │ NeoPixel.setColor(R,0,0) │
  │ LED turns RED            │
  └──────────────────────────┘
```

### 🔐 Lợi Ích Của Rule Chain

✅ **Tự động hóa thông minh** - Không cần can thiệp thủ công  
✅ **Giao tiếp giữa thiết bị** - Liên kết hành động của nhiều thiết bị  
✅ **Tiết kiệm năng lượng** - Chỉ bật thiết bị khi cần  
✅ **Theo dõi từ xa** - Kiểm soát toàn bộ hệ thống từ Cloud  
✅ **Dễ mở rộng** - Có thể thêm nhiều rule mới mà không cần thay đổi firmware  

---

## 📱 Cấu Hình WiFi & Access Token

### 🔐 Vị Trí Cấu Hình

Tất cả cài đặt WiFi và Access Token được lưu trong file: **`src/global.cpp`**

### 📍 Cách Nhập WiFi SSID

**File**: `src/global.cpp` (dòng ~6)

```cpp
String wifi_ssid = "Taolabochungmay";  // ← Thay đổi tên WiFi ở đây
```

**Cách làm:**
1. Mở file `src/global.cpp`
2. Tìm dòng: `String wifi_ssid = "Taolabochungmay";`
3. Thay `"Taolabochungmay"` bằng tên WiFi của bạn
   - Ví dụ: `String wifi_ssid = "MyHomeWiFi";`

### 🔒 Cách Nhập WiFi Password

**File**: `src/global.cpp` (dòng ~7)

```cpp
String wifi_password = "Ronaldoisgoat";  // ← Thay đổi mật khẩu WiFi ở đây
```

**Cách làm:**
1. Mở file `src/global.cpp`
2. Tìm dòng: `String wifi_password = "Ronaldoisgoat";`
3. Thay `"Ronaldoisgoat"` bằng mật khẩu WiFi của bạn
   - Ví dụ: `String wifi_password = "MySecurePassword123";`

### 🔑 Cách Nhập CoreIOT Access Token

**File**: `src/global.cpp` (dòng ~8)

```cpp
String CORE_IOT_TOKEN = "GEWwhpnyPx2CghKvofB8";  // ← Thay đổi token ở đây
```

**Cách lấy Access Token từ CoreIOT:**
1. Đăng nhập vào [CoreIOT Dashboard](https://app.coreiot.io)
2. Vào mục **"Devices"** → Chọn device của bạn
3. Tab **"Details"** → Sao chép **"Access Token"**
4. Dán token vào `src/global.cpp`
   - Ví dụ: `String CORE_IOT_TOKEN = "YOUR_TOKEN_HERE";`

### ✅ Ví Dụ Cấu Hình Hoàn Chỉnh

```cpp
// File: src/global.cpp
#include "global.h"

float glob_temperature = 0.0;
float glob_humidity = 0.0;

String wifi_ssid = "MyNetwork";              // 🔧 Nhập WiFi SSID
String wifi_password = "MyPassword123";      // 🔧 Nhập WiFi Password  
String CORE_IOT_TOKEN = "abc123def456...";   // 🔧 Nhập Access Token

SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();
```

### 📡 MQTT Server Info

- **Broker**: `app.coreiot.io`
- **Port**: `1883`
- **Topics**:
  - `v1/devices/me/telemetry` - Gửi dữ liệu cảm biến
  - `v1/devices/me/rpc/request/+` - Nhận lệnh RPC

### ⚠️ Lưu Ý Quan Trọng

- ⚡ **Sau khi thay đổi thông tin**, bạn phải **biên dịch lại** (`platformio run`) và **upload** vào board
- 🔐 **Không chia sẻ** Access Token với người khác
- ✔️ **Kiểm tra** SSID/Password/Token đúng chính xác trước khi upload
- 📱 Tên WiFi phải khớp **CHÍNH XÁC** (bao gồm cả chữ hoa/thường)

## 💾 Cấu Trúc Project

```
YoloUNO_PlatformIO-LED_Blinky/
├── src/
│   ├── main.cpp                 # Entry point, khởi tạo tasks
│   ├── global.cpp               # Biến global, thông tin WiFi/MQTT
│   ├── mainServer.cpp           # WiFi setup (setupSTAMode)
│   ├── task_led.cpp             # LED control task
│   ├── task_temp_humid.cpp      # DHT20 sensor reading
│   └── task_coreIOT.cpp         # MQTT & RPC callback
├── include/
│   ├── global.h                 # Khai báo biến global
│   ├── mainServer.h             # Function prototypes
│   ├── task_led.h
│   ├── task_temp_humid.h
│   └── task_coreIOT.h
├── lib/
│   └── DHT20/                   # Custom DHT20 library
├── boards/
│   └── yolo_uno.json            # Board configuration
└── platformio.ini               # PlatformIO project config
```

## 🔌 Sơ Đồ Kết Nối

```
YoloUNO (ESP32-S3)
├── GPIO11 (SDA) ─── DHT20 (I2C data)
├── GPIO12 (SCL) ─── DHT20 (I2C clock)
├── GPIO48 ─────────── LED (Output)
│
├── WiFi (2.4GHz) ─── Router ─── Internet
│
└── MQTT ────────────── app.coreiot.io:1883
```

## 📤 Dữ Liệu Gửi Lên Server

### Telemetry (mỗi 5 giây)
```json
{
  "temperature": 28.5,
  "humidity": 65.3
}
```

## 📥 Lệnh RPC Từ Server

### Điều khiển LED
```json
{
  "method": "setValue",
  "params": {"led": 1}
}
```

Response:
```json
{"result": "ok"}
```

### Lấy giá trị cảm biến
```json
{
  "method": "getSensor",
  "params": {}
}
```

Response:
```json
{
  "temperature": 28.5,
  "humidity": 65.3
}
```

## 🛠️ Biên Dịch & Upload

### Build
```bash
platformio run
```

### Upload
```bash
platformio run --target upload
```

### Monitor Serial Output
```bash
platformio device monitor
```

## 📊 Serial Monitor Output Mẫu

```
Starting WiFi connection...
Connecting to: Taolabochungmay
.....
WiFi connected successfully!
IP address: 192.168.1.100
Signal strength (RSSI): -65

Temperature: 28.5 °C, Humidity: 65.3 %
Attempting MQTT connection...
connected to server
Data published{"temperature":28.5,"humidity":65.3}
```

## ⚙️ Cấu Hình PlatformIO

```ini
[env:yolo_uno]
platform = espressif32
board = yolo_uno
framework = arduino
monitor_speed = 115200

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
```

## 🐛 Troubleshooting

### I2C Error: "i2cRead returned Error -1"
- ✅ Kiểm tra kết nối DHT20: VDD (3.3V), SDA (GPIO11), GND, SCL (GPIO12)
- ✅ Thêm pull-up resistor 10kΩ trên SDA/SCL nếu cần

### WiFi không kết nối
- ✅ Kiểm tra SSID & password trong global.cpp
- ✅ Xem Serial Monitor để biết lỗi WiFi status code

### MQTT Connection Failed
- ✅ Kiểm tra token CoreIOT: `CORE_IOT_TOKEN`
- ✅ Đảm bảo thiết bị đã được thêm vào CoreIOT dashboard

## 👨‍💻 Tác Giả

Dự án được phát triển cho khóa HK252 - Phát Triển Ứng Dụng IoT

## 📄 License

MIT License - Tự do sử dụng và phát triển


1. [Install PlatformIO](https://platformio.org)
2. Create PlatformIO project and configure a platform option in [platformio.ini](https://docs.platformio.org/page/projectconf.html) file:

## Stable version

See `platform` [documentation](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/platform.html#projectconf-env-platform) for details.

```ini
[env:stable]
; recommended to pin to a version, see https://github.com/platformio/platform-espressif32/releases
; platform = espressif32 @ ^6.0.1
platform = espressif32
board = yolo_uno
framework = arduino
monitor_speed = 115200

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

## Development version

```ini
[env:development]
platform = https://github.com/platformio/platform-espressif32.git
board = yolo_uno
framework = arduino
monitor_speed = 115200
build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

    
# Configuration

Please navigate to [documentation](https://docs.platformio.org/page/platforms/espressif32.html).