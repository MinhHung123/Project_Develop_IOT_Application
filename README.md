# Smart Home trên YoloUNO — BTL Phát triển ứng dụng IoT (CO3037)

Hệ thống nhà thông minh chạy trên kit **YoloUNO (ESP32-S3)**, hiện thực 7 task FreeRTOS:
giám sát môi trường, điều khiển thiết bị tại chỗ, cấu hình WiFi qua trang web, suy luận
TinyML on-device, đẩy telemetry lên CoreIOT Cloud và điều khiển ngược qua MQTT RPC.

**Môn:** Phát triển ứng dụng IoT (CO3037) — HCMUT, Khoa KH&KTMT
**GVHD:** TS. Lê Trọng Nhân
**Nhóm:** Thạch Minh Hưng (2311357), Nguyễn Lê Khôi Nguyên (2312366)

---

## 1. Tổng quan hệ thống

### Mục tiêu

Mở rộng `nhanksd85/YoloUNO_PlatformIO @ RTOS_Project` thành một hệ thống smart home đầy
đủ pipeline: cảm biến → xử lý cục bộ (kể cả ML) → hiển thị → cloud → điều khiển ngược.
Mức khác base ≥ 30%.

### 7 task chính (theo đề bài)

| # | Task                          | File                                       | Đề bài yêu cầu                                                   |
|---|-------------------------------|--------------------------------------------|------------------------------------------------------------------|
| 1 | LED blink theo nhiệt độ       | `task_led_blink.{h,cpp}`                   | ≥3 behaviors theo dải nhiệt, đồng bộ bằng semaphore              |
| 2 | NeoPixel theo độ ẩm           | `task_neo_led.{h,cpp}`                     | ≥3 mức màu theo humidity, semaphore                              |
| 3 | LCD hiển thị temp+humid       | `task_lcd.{h,cpp}`                         | ≥3 trạng thái, không dùng global var, dùng semaphore             |
| 4 | Web server AP mode            | `mainServer.{h,cpp}`                       | UI redesign, ≥2 device control, chế độ AP+STA thông minh         |
| 5 | TinyML anomaly detection      | `task_tinyml.{h,cpp}` + `ml_model/`        | Train + deploy model 3-class, đánh giá accuracy trên HW          |
| 6 | CoreIOT publish (MQTT)        | `task_coreIOT.{h,cpp}`                     | STA mode, publish telemetry                                      |
| 7 | Rule Chain trên CoreIOT       | (cấu hình trên Cloud, không phải code)    | Master + Slave chain, telemetry → RPC                            |

### Phân công

| Thành viên                | MSSV    | Tasks                                |
|---------------------------|---------|--------------------------------------|
| Thạch Minh Hưng           | 2311357 | Task 2, 4, 6, 7                      |
| Nguyễn Lê Khôi Nguyên     | 2312366 | Task 1, 3, 5                         |

---

## 2. Phần cứng

| Thành phần                     | Pin / Địa chỉ                       | Chú thích                          |
|--------------------------------|-------------------------------------|------------------------------------|
| Board YoloUNO (ESP32-S3)       | —                                   | Dual-core Xtensa LX7 240 MHz       |
| I2C bus                        | SDA = **GPIO 11**, SCL = **GPIO 12** | Dùng chung DHT20 + LCD              |
| DHT20 (nhiệt độ + độ ẩm)       | I2C @ 0x38                          | Đọc mỗi 2 s                        |
| LCD I2C 16×2 (HD44780+PCF8574) | I2C @ **0x21**                      | KHÔNG phải 0x27                    |
| LED đơn (on-board)             | **GPIO 48**                         | Active HIGH                        |
| NeoPixel WS2812                | **GPIO 45**                         | 1 LED RGB                          |
| Servo cửa                      | **GPIO 5**                          | `DOOR_SERVO_PIN`                   |
| Quạt DC                        | **GPIO 10**                         | `FAN_PIN`, PWM qua `analogWrite`   |
| Button                         | **GPIO 8** (BTN1), **GPIO 9** (BTN2)| INPUT_PULLUP, active-low           |

---

## 3. Kiến trúc phần mềm

### 3.1 Sơ đồ task FreeRTOS

```
┌────────────────────────────────────────────────────────────────┐
│                       ESP32-S3 (YoloUNO)                       │
│                                                                │
│  ┌─────────────────┐   sensor_write()   ┌──────────────────┐  │
│  │ task_temp_humid │ ─────────────────► │  SensorData      │  │
│  │  (prio = 2)     │                    │  + xMutexSensor  │  │
│  │  DHT20 @ 2 Hz   │                    │  Data (mutex)    │  │
│  └────────┬────────┘                    └────────┬─────────┘  │
│           │                                       │            │
│           │ xSemaphoreGive() x4                   │ sensor_    │
│           │ (separate sem per consumer)           │ read()     │
│           ▼                                       │            │
│  ┌────────────────┐  ┌────────────────┐  ┌──────▼──────────┐  │
│  │ xSem_Led       │  │ xSem_Lcd       │  │ xSem_TinyML     │  │
│  └───────┬────────┘  └───────┬────────┘  └───────┬─────────┘  │
│          ▼                   ▼                   ▼            │
│  ┌────────────────┐  ┌────────────────┐  ┌─────────────────┐  │
│  │ task_led_blink │  │ task_lcd_      │  │ task_tinyml_    │  │
│  │  (prio = 1)    │  │ display        │  │ inference       │  │
│  │  GPIO 48       │  │ LCD I2C 0x21   │  │ TFLite Micro    │  │
│  └────────────────┘  └────────────────┘  └────────┬────────┘  │
│                                                    │           │
│           ┌────────────────┐                       │ writes    │
│           │ xSem_Temp_Humi │ ─► TaskNEOLED         │ globs     │
│           └────────────────┘    GPIO 45            ▼           │
│                                                                │
│  ┌──────────────────┐  ┌───────────────┐  ┌─────────────────┐ │
│  │ TaskWebServer    │  │ TaskCoreIOT   │  │ task_button     │ │
│  │ AP+STA, port 80  │  │ MQTT publish  │  │ BTN1, BTN2      │ │
│  │ /, /wifi, /fan,  │  │ + RPC callback│  │ debounce 30 ms  │ │
│  │ /door, /sensor-  │  │ FAN_PIN,      │  │ toggle fan      │ │
│  │ state            │  │ DOOR_SERVO    │  │                 │ │
│  └────────┬─────────┘  └──────┬────────┘  └─────────────────┘ │
│           │                   │                                │
│           │ xSem_Internet     │ xSem_FanUpdate                 │
│           └───────────────────┘                                │
│                                                                │
└──────────────────────┬─────────────────────────────────────────┘
                       │ MQTT 1883
                       ▼
              ┌───────────────────────┐
              │ CoreIOT Cloud         │
              │ app.coreiot.io        │
              │ Rule Chain (Task 7)   │
              │ Dashboard widgets     │
              └───────────────────────┘
```

### 3.2 Bảng task

| Task                       | Stack | Prio | Vai trò                              |
|----------------------------|-------|------|--------------------------------------|
| `task_temp_humid`          | 2 KB  | 2    | Producer DHT20 mỗi 2 s               |
| `task_button`              | 2 KB  | 2    | Polling button + debounce            |
| `task_led_blink`           | 2 KB  | 1    | Consumer LED (Task 1)                |
| `TaskNEOLED`               | 2 KB  | 1    | Consumer NeoPixel (Task 2)           |
| `task_lcd_display`         | 4 KB  | 1    | Consumer LCD (Task 3)                |
| `task_tinyml_inference`    | 8 KB  | 1    | TFLite Micro inference (Task 5)      |
| `TaskCoreIOT`              | 4 KB  | 1    | MQTT publish + RPC (Task 6)          |
| `TaskWebServer`            | 6 KB  | 1    | HTTP server + AP fallback (Task 4)   |

> Stack thực dùng (đo qua `uxTaskGetStackHighWaterMark`) thấp hơn stack cấp khoảng 30–65%,
> chi tiết xem Mục 4.7 báo cáo.

### 3.3 Cơ chế đồng bộ — ADR-03 và ADR-04

Project tuân theo **phương án lai (hybrid)** để vừa đáp ứng yêu cầu Task 3 ("không dùng
global variable"), vừa không phá vỡ code teammate cũ:

1. **`SensorData` struct + mutex** (chuẩn cho code mới):
   ```cpp
   struct SensorData {
       float temp;
       float humi;
       uint32_t timestamp_ms;
       bool valid;
   };
   ```
   Truy cập qua `sensor_read()` / `sensor_write()` (atomic, bảo vệ bằng
   `xMutexSensorData`). Task 1, 3, 5 dùng API này.

2. **Semaphore riêng cho mỗi consumer** (tránh stolen wake-up):
   - `xBinarySemaphoreLed` cho Task 1
   - `xBinarySemaphoreLcd` cho Task 3
   - `xBinarySemaphoreTinyML` cho Task 5
   - `xBinarySemaphoreTemp_Humi` cho Task 2 (legacy, dùng glob_humidity)

3. **Global cũ** (`glob_temperature`, `glob_humidity`) vẫn được giữ cho code Task 2,
   4, 6 đã viết trước đó. Task DHT20 ghi cả hai (struct + global) để đảm bảo
   compatibility.

4. **Biến `volatile`** cho kết quả TinyML (`glob_ml_label`, `glob_ml_confidence`):
   Task 5 ghi, Task 6 đọc để publish lên MQTT. Sentinel `-1` để bỏ qua publish
   trước khi inference đầu tiên xảy ra.

---

## 4. Chi tiết từng Task

### Task 1 — LED blink theo nhiệt độ

LED đơn tại **GPIO 48**. Consumer của `xBinarySemaphoreLed`, đọc nhiệt độ qua
`sensor_read()`, không tự đọc cảm biến.

Ba dải hành vi:

| Dải nhiệt độ          | Trạng thái  | Nửa chu kỳ | Nhịp/wake (2 s) |
|-----------------------|-------------|------------|-----------------|
| T < 25 °C             | COLD (chậm) | 1000 ms    | 1               |
| 25 ≤ T < 35 °C        | NORMAL (vừa)| 500 ms     | 2               |
| T ≥ 35 °C             | HOT (nhanh) | 150 ms     | 7               |

**Pattern blocking burst**: mỗi lần thức dậy nháy N nhịp (≈2 s) thay vì 1 nhịp/wake,
để dải HOT nhìn thấy rõ khác NORMAL bằng mắt thường.

### Task 2 — NeoPixel theo độ ẩm

NeoPixel WS2812 tại **GPIO 45**. Consumer của `xBinarySemaphoreTemp_Humi`, đọc
`glob_humidity`.

| Độ ẩm        | Màu       |
|--------------|-----------|
| RH < 30%     | 🔴 ĐỎ     |
| 30% ≤ RH < 60% | 🟡 VÀNG  |
| RH ≥ 60%     | 🟢 XANH LÁ|

### Task 3 — LCD hiển thị temp+humid

LCD I2C 16×2 @ **0x21** (KHÔNG phải 0x27 mặc định của OhStem — đã verify bằng I2C
scanner). Consumer của `xBinarySemaphoreLcd`, đọc qua `sensor_read()`.

**Hai màn hình xoay vòng** mỗi 4 s (2 chu kỳ producer):
- **Screen Sensor**: `T:28.3 H:61.2%` / `STATUS: NORMAL`
- **Screen Devices**: `DOOR:CLS FAN:OFF` / (placeholder)

**Ba trạng thái** (ưu tiên CRITICAL > NORMAL > WARNING):

| Trạng thái  | Điều kiện                                                          |
|-------------|--------------------------------------------------------------------|
| NORMAL      | 20 ≤ T ≤ 30 °C **và** 30% ≤ H ≤ 70%                                |
| WARNING     | Ngoài NORMAL nhưng chưa chạm CRITICAL                              |
| CRITICAL    | T < 15 hoặc T > 35 °C, **hoặc** H < 20% hoặc H > 85%               |

**Bộ lọc EMA** (α = 0.3) làm mượt giá trị hiển thị; **partial update** so sánh từng
dòng với buffer để tránh flicker khi gọi `lcd.clear()` mỗi chu kỳ.

**Fail-safe**: khi `sensor_read()` trả về `false` hoặc `valid == false`, hiển thị
`T:--.- H:--.-%` / `STATUS: SENS ERR`.

### Task 4 — Web Server (AP + STA mode)

Webserver HTTP trên port 80, hoạt động ở chế độ **AP+STA đồng thời**:

- **AP**: SSID `ESP32-Config`, pass `12345678`, IP `192.168.4.1` — luôn bật khi mất
  STA, dùng để cấu hình WiFi tại chỗ.
- **STA**: kết nối tới router gia đình bằng SSID/password đã nhập qua form.

**Routes**:

| Route             | Method | Mô tả                                                |
|-------------------|--------|------------------------------------------------------|
| `/`               | GET    | Dashboard HTML/CSS/JS (glass theme, dark mode)       |
| `/wifi`           | POST   | Nhận SSID + password, trigger STA reconnect          |
| `/retry`          | GET    | Thử kết nối lại STA                                  |
| `/fan?speed=N`    | GET    | Set tốc độ quạt 0–100%, ánh xạ → PWM 0–255           |
| `/fan-state`      | GET    | JSON `{"speed": N}`                                  |
| `/door?action=…`  | GET    | `open` / `close` / `toggle` cửa                      |
| `/door-state`     | GET    | JSON `{"open": bool, "angle": N}`                    |
| `/sensor-state`   | GET    | JSON `{"temperature": T, "humidity": H}`             |

Dashboard chứa **biểu đồ realtime** (HTML5 Canvas) cho nhiệt độ + độ ẩm, polling
mỗi 1–2 s qua `fetch()`.

### Task 5 — TinyML anomaly detection (3-class)

Pipeline end-to-end **3 phase**:

```
Phase A (Python)         Phase B (Python)              Phase C (ESP32)
─────────────────        ───────────────────           ──────────────────
generate_dataset.py  →   train_and_export.py     →    task_tinyml.cpp
↓                        ↓                              ↓
3160 mẫu CSV             MLP 2→16→8→3                   TFLite Micro
synthetic                Keras → TFLite                 Inference @ 2 Hz
(NORMAL/WARNING/         dynamic-range quant            Latency 11–13 ms
CRITICAL)                3972 byte .tflite              Arena 8 KB → dùng ~2 KB
                         → C header
```

**Kết quả thực nghiệm** (xem `notebooks/metrics.txt`):

| Chỉ số                          | Giá trị     |
|---------------------------------|-------------|
| Test accuracy (Keras float)     | **99.53%**  |
| Macro F1                        | **0.9945**  |
| TFLite agreement vs Keras float | **1.0000**  |
| Kích thước `.tflite`            | 3972 byte   |
| Latency suy luận trên ESP32-S3  | 11–13 ms    |

**Ngưỡng phân lớp** (script `scripts/generate_dataset.py`):

| Lớp       | T (°C)               | H (%)                  |
|-----------|----------------------|------------------------|
| NORMAL    | 22–30                | 40–75                  |
| WARNING   | 18–22 hoặc 30–35     | 25–40 hoặc 75–85       |
| CRITICAL  | <18 hoặc >35         | <25 hoặc >85           |

**Kiến trúc model**:
```
Input(2) → Normalization (adapted on X_train, embedded in graph)
        → Dense(16, ReLU) → Dense(8, ReLU)
        → Dense(3, softmax)
```
Tổng 216 params; vì Normalization layer được nhúng trong graph, ESP32 đẩy thẳng
giá trị thô từ DHT20 vào tensor input — không cần chuẩn hóa thủ công, loại bỏ
train-serve skew.

Kết quả ML được publish lên Cloud cùng với telemetry:
```json
{
  "fan_speed": 50,
  "temperature": 28.3,
  "humidity": 61.2,
  "ml_label": 0,
  "ml_state": "NORMAL",
  "ml_confidence": 0.998
}
```

### Task 6 — CoreIOT publish + RPC (MQTT)

MQTT client kết nối tới `app.coreiot.io:1883` với token trong `CORE_IOT_TOKEN`.
ClientId tạo từ MAC duy nhất (`ESP.getEfuseMac()`) để tránh ngắt kết nối tuần
hoàn khi 2 kit chạy song song.

**Telemetry uplink** (mỗi 2 s, hoặc khi `xBinarySemaphoreFanUpdate` được give):
- Topic: `v1/devices/me/telemetry`
- Payload: JSON như ví dụ Task 5 ở trên.

**RPC downlink** (subscribe `v1/devices/me/rpc/request/+`):

| Method   | Params         | Hành động                                          |
|----------|----------------|----------------------------------------------------|
| `setFan` | int (0–100)    | Set tốc độ quạt, xuất PWM ra `FAN_PIN`            |
| `setDoor`| —              | Toggle servo 0° ↔ 90° (cửa đóng/mở)               |

Sau khi xử lý RPC, hàm `updateAndPulish()` gọi `publishSensor()` ngay để đồng bộ
trạng thái lên Dashboard.

**Đồng bộ với Wi-Fi**: nếu mất STA, `xBinarySemaphoreInternet` được drain để
TaskCoreIOT pause; khi STA quay lại, semaphore được give, MQTT reconnect.

### Task 7 — Rule Chain trên CoreIOT (Cloud config)

**Không phải code trên ESP32** — Task 7 là cấu hình rule chain trực tiếp trên
web UI của CoreIOT, dạng kéo-thả các Rule Node.

**Master + Slave chain**:
- **Master**: nhận telemetry từ Device A, lọc theo điều kiện (ví dụ
  `humidity > 60`), đổi originator sang Device B, đẩy sang Slave chain.
- **Slave**: nhận message đã re-route, kiểm tra attribute, gọi RPC `setLed`
  (hoặc `setColor`) tới Device B.

Demo cụ thể: Device A đọc humidity > 60% → Cloud tự động kích RPC bật LED đỏ
trên Device B.

---

## 5. Cấu trúc project

```
Project_Develop_IOT_Application/
├── platformio.ini                  # PlatformIO config
├── boards/
│   └── yolo_uno.json               # Board definition
├── lib/
│   └── DHT20/                      # Local DHT20 driver
├── include/
│   ├── global.h                    # SensorData struct + mutex + 4 sem
│   ├── task_temp_humid.h           # DHT20 producer
│   ├── task_led_blink.h            # Task 1 — LED
│   ├── task_neo_led.h              # Task 2 — NeoPixel
│   ├── task_lcd.h                  # Task 3 — LCD
│   ├── mainServer.h                # Task 4 — WebServer
│   ├── task_tinyml.h               # Task 5 — TinyML
│   ├── task_coreIOT.h              # Task 6 — MQTT
│   ├── button.h                    # Button handler
│   └── ml_model/
│       └── dht_classifier_model.h  # 3972-byte TFLite (C array)
├── src/
│   ├── main.cpp                    # setup() + xTaskCreate x8
│   ├── global.cpp                  # Định nghĩa biến + sensor_read/write
│   ├── task_temp_humid.cpp
│   ├── task_led_blink.cpp
│   ├── task_neo_led.cpp
│   ├── task_lcd.cpp
│   ├── mainServer.cpp
│   ├── task_tinyml.cpp
│   ├── task_coreIOT.cpp
│   └── button.cpp
├── scripts/
│   └── generate_dataset.py         # Phase A: sinh dataset synthetic
├── notebooks/
│   ├── train_and_export.py         # Phase B: train + quantize + export
│   ├── metrics.txt                 # Kết quả accuracy, F1, confusion matrix
│   └── plots/
│       ├── dataset_scatter.png
│       ├── training_curves.png
│       └── confusion_matrix.png
├── dataset/
│   └── dht_anomaly_dataset.csv     # 3160 mẫu synthetic
└── README.md
```

---

## 6. Cấu hình & nạp firmware

### 6.1 Cài đặt WiFi và CoreIOT token

File **`src/global.cpp`** (dòng ~9–11):

```cpp
String wifi_ssid     = "";                       // ← để trống nếu cấu hình qua AP
String wifi_password = "";
String CORE_IOT_TOKEN = "GEWwhpnyPx2CghKvofB8";  // ← token CoreIOT
```

Hai cách cấu hình WiFi:

1. **Sửa cứng**: nhập SSID/password trực tiếp vào `global.cpp` rồi build lại.
2. **Cấu hình tại chỗ (khuyến nghị)**: để trống, ESP32 sẽ tự bật AP
   `ESP32-Config` (pass `12345678`). Kết nối điện thoại/laptop vào AP này, mở
   trình duyệt `http://192.168.4.1`, nhập SSID/password vào form `/wifi`.

### 6.2 Lấy CoreIOT token

1. Đăng nhập [app.coreiot.io](https://app.coreiot.io)
2. Devices → chọn device → Tab "Details" → Copy "Access Token"
3. Paste vào `CORE_IOT_TOKEN`

### 6.3 Build & upload (PlatformIO)

```bash
# Build
platformio run

# Upload
platformio run --target upload

# Serial monitor
platformio device monitor
```

### 6.4 Pipeline TinyML (tùy chọn — model đã có sẵn)

```bash
# Phase A: sinh dataset (chạy từ project root)
python3 scripts/generate_dataset.py
# → dataset/dht_anomaly_dataset.csv, notebooks/plots/dataset_scatter.png

# Phase B: train + quantize + export C header
python3 notebooks/train_and_export.py
# → include/ml_model/dht_classifier_model.h, notebooks/metrics.txt, plots/

# Phase C: rebuild firmware (đã có header mới)
platformio run --target upload
```

> Phải dùng **TensorFlow 2.x** (đã test với 2.21.0) để export đúng schema
> TFLite mà thư viện `tanakamasayuki/TensorFlowLite_ESP32@1.0.0` hỗ trợ.

---

## 7. PlatformIO config

```ini
[env:yolo_uno]
platform = espressif32
board = yolo_uno
framework = arduino
monitor_speed = 115200

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

lib_deps =
    ArduinoHttpClient
    ArduinoJson
    DHT20
    LCD
    PubSubClient@2.8.0
    Thingsboard@0.13.0
    adafruit/Adafruit NeoPixel @ ^1.15.4
    madhephaestus/ESP32Servo @ ^1.1.8
    tanakamasayuki/TensorFlowLite_ESP32@1.0.0
```

---

## 8. Hiệu năng & tài nguyên

Đo trên kit Yolo UNO (ESP32-S3, 4 MB Flash, 8 MB PSRAM) chạy đủ 8 task đồng thời
trong 10 phút (xem chi tiết Mục 4.7 báo cáo PDF):

| Chỉ số                             | Giá trị        | So với ngân sách      |
|------------------------------------|----------------|-----------------------|
| Flash usage                        | ≈ 1.1 MB       | 27% của 4 MB          |
| RAM usage (stack + heap)           | ≈ 78 KB        | 15% của 512 KB        |
| Tổng stack 8 task (cấp phát)       | 32 KB          | —                     |
| TFLite tensor arena                | 8 KB (cấp) / 2 KB (dùng) | —          |
| PSRAM                              | Không dùng     | 0% của 8 MB           |
| Latency wake-up consumer           | < 1 ms         | < 5 ms (target)       |
| Latency inference TinyML           | 11–13 ms       | < 50 ms (target)      |
| MQTT publish round-trip            | 80–150 ms      | —                     |
| RPC command response               | 100–200 ms     | < 500 ms (target)     |

---

## 9. Sự cố đã gặp & cách xử lý

| Sự cố                                       | Triệu chứng                              | Nguyên nhân                                          | Khắc phục                                                                        |
|---------------------------------------------|------------------------------------------|------------------------------------------------------|----------------------------------------------------------------------------------|
| Stack canary watchpoint (Task 1)            | Reset ngay sau `Serial.printf`           | Stack 2 KB không đủ cho buffer của `printf`          | Tăng stack lên 4 KB                                                              |
| Stolen wake-up giữa Task 1, 3, 5            | Consumer bỏ lỡ sự kiện                   | Dùng chung 1 binary semaphore cho nhiều consumer     | Cấp semaphore riêng cho từng consumer (Cách 1 của ADR-04)                        |
| TFLite Micro schema mismatch                | `model schema mismatch`                  | TF version khác với phiên bản thư viện hỗ trợ        | Downgrade TF training về version tương thích                                     |
| MQTT publish giá trị rác `ml_label = -1`    | Dashboard hiển thị UNKNOWN trong 2 s đầu | Task 6 publish trước khi Task 5 hoàn thành infer     | Sentinel `-1` + check `if (glob_ml_label >= 0)` trong `publishSensor()`          |
| MQTT disconnect tuần hoàn `rc=-2`           | 2 kit chạy song song bị ngắt liên tục    | `clientId` trùng                                     | Tạo `clientId` từ `ESP.getEfuseMac()`                                            |
| Tensor arena ban đầu thiếu                  | `AllocateTensors()` trả `kTfLiteError`   | Arena 4 KB không đủ overhead nội tại của TFLite Micro | Tăng lên 8 KB; log `arena_used_bytes()` báo dùng ~2 KB                           |
| WebServer treo khi mất WiFi                 | Hệ thống đóng băng, response chậm        | Hàm blocking trong PubSubClient                      | Drain semaphore `xBinarySemaphoreInternet` để MQTT tự pause khi mất mạng         |

---

## 10. Hạn chế & hướng phát triển

### Hạn chế hiện tại

- **Dataset Task 5 là synthetic** — chưa thu thập dữ liệu thực dài hạn từ DHT20.
  Accuracy 99.53% phản ánh khớp với logic gán nhãn synthetic, chưa khẳng định
  khả năng tổng quát hóa.
- **Mô hình chỉ dùng 2 feature tức thời** (T, H) — không khai thác temporal
  feature (ΔT, ΔH) vốn quan trọng để phát hiện cháy, rò rỉ khí.
- **MQTT chưa mã hóa** (port 1883, không TLS) — payload và token đi qua mạng
  ở plain text.
- **Dashboard HTML nhúng cứng** trong `mainServer.cpp` dưới dạng `String html`
  — khó bảo trì CSS/JS, tốn Flash.
- **Ngưỡng phân loại hard-code** ở cả Task 3 (LCD) và Task 5 (training script)
  — phải rebuild khi đổi.

### Hướng phát triển

- TLS/SSL cho MQTT, OAuth/JWT cho WebServer.
- Thu thập dữ liệu real từ DHT20 trong nhiều tuần, fine-tune model trên hỗn hợp
  synthetic + real.
- Bổ sung feature ΔT, ΔH hoặc nâng lên 1D-CNN trên cửa sổ 5–10 mẫu.
- Đưa threshold thành CoreIOT attribute để cấu hình từ xa.
- Tách Dashboard HTML ra file riêng, dùng SPIFFS hoặc LittleFS.
- Tích hợp ESP-NOW / BLE Mesh để Rule Chain hoạt động ngay cả khi mất internet.

---

## 11. Tham khảo

- ThingsBoard documentation: <https://thingsboard.io/docs/>
- CoreIOT documentation: <https://coreiot.io/docs/>
- TensorFlow Lite for Microcontrollers: <https://www.tensorflow.org/lite/microcontrollers>
- OhStem YoloUNO: <https://ohstem.vn/product/yolo-uno/>
- OhStem DHT20: <https://docs.ohstem.vn/en/latest/module/cam-bien/dht20.html>
- OhStem LCD 1602 I2C: <https://docs.ohstem.vn/en/latest/module/hien-thi/lcd1602.html>
- Random Nerd Tutorials — ESP32 Web Server: <https://randomnerdtutorials.com/esp32-web-server-beginners-guide/>

---

## License

MIT — tự do sử dụng và phát triển. Báo cáo PDF + source code là sản phẩm BTL,
mọi tái sử dụng học thuật nên trích nguồn nhóm.
