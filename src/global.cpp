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
