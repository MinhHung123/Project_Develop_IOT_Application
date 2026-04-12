# YoloUNO IoT Platform Project

Dự án IoT hoàn chỉnh sử dụng **ESP32-S3 (YoloUNO)** để kết nối với **CoreIOT** qua MQTT, điều khiển LED, đọc cảm biến nhiệt độ/độ ẩm, và nhận lệnh RPC từ server.

## 📋 Tính Năng Chính

✅ **Kết nối WiFi Station Mode** - Tự động kết nối đến WiFi network  
✅ **Cảm biến DHT20** - Đọc nhiệt độ và độ ẩm qua I2C  
✅ **Điều khiển LED** - Task FreeRTOS điều khiển LED blink  
✅ **MQTT CoreIOT Integration** - Gửi/nhận dữ liệu với cloud  
✅ **RPC Command Processing** - Nhận và xử lý lệnh từ server  
✅ **Multi-tasking với FreeRTOS** - 3 tasks chạy song song  

## 🔧 Phần Cứng

- **Board**: YoloUNO (ESP32-S3)
- **Cảm biến**: DHT20 (I2C - SDA: GPIO11, SCL: GPIO12)
- **Output**: LED trên GPIO48
- **Kết nối**: WiFi 2.4GHz

## 📦 Thư Viện Sử Dụng

```
- ArduinoJson (JSON parsing)
- PubSubClient@2.8.0 (MQTT client)
- Thingsboard@0.13.0 (CoreIOT integration)
- ArduinoHttpClient (HTTP client)
- DHT20 (Temperature/Humidity sensor)
```

## 🚀 Cụuc Trình Khởi Động

1. **Serial Communication** - 115200 baud
2. **WiFi Setup** - Kết nối với SSID/Password từ global.cpp
3. **I2C Initialization** - Chuẩn bị đọc DHT20
4. **FreeRTOS Tasks**:
   - `TempHumidTask` (Priority 1) - Đọc cảm biến mỗi 2 giây
   - `LED Control` (Priority 2) - Blink LED mỗi 2 giây
   - `CoreIOT Task` (Priority 3) - MQTT connection & RPC handling

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