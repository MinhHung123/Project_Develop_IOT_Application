#include "global.h"

float glob_temperature = 0.0;
float glob_humidity = 0.0;

String wifi_ssid = "Taolabochungmay";
String wifi_password = "Ronaldoisgoat";

String CORE_IOT_TOKEN = "GEWwhpnyPx2CghKvofB8";
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();