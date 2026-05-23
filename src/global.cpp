#include "global.h"

float glob_temperature = 0.0;
float glob_humidity = 0.0;
int fan_speed = 0;
Servo myServo;
int pos_servo = 0;

String wifi_ssid = "";
String wifi_password = "";

String CORE_IOT_TOKEN = "GEWwhpnyPx2CghKvofB8";
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();
SemaphoreHandle_t xBinarySemaphoreTemp_Humi = xSemaphoreCreateBinary();
SemaphoreHandle_t xBinarySemaphoreFanUpdate = xSemaphoreCreateBinary();

SemaphoreHandle_t xMutexSensorData    = xSemaphoreCreateMutex();
SemaphoreHandle_t xBinarySemaphoreLed = xSemaphoreCreateBinary();
SemaphoreHandle_t xBinarySemaphoreLcd = xSemaphoreCreateBinary();
SemaphoreHandle_t xBinarySemaphoreTinyML = xSemaphoreCreateBinary();

volatile int   glob_ml_label      = -1;
volatile float glob_ml_confidence = 0.0f;

// File-scope storage; access only via sensor_read/sensor_write.
static SensorData s_sensor_data = {0.0f, 0.0f, 0, false};

bool sensor_read(SensorData* out) {
    if (out == nullptr) return false;
    if (xSemaphoreTake(xMutexSensorData, pdMS_TO_TICKS(50)) != pdTRUE) return false;
    *out = s_sensor_data;
    xSemaphoreGive(xMutexSensorData);
    return out->valid;
}

bool sensor_write(const SensorData& in) {
    if (xSemaphoreTake(xMutexSensorData, pdMS_TO_TICKS(50)) != pdTRUE) return false;
    s_sensor_data = in;
    xSemaphoreGive(xMutexSensorData);
    return true;
}