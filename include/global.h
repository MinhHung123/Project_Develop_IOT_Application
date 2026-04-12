#ifndef GLOBAL_H
#define GLOBAL_H

// Global variables and function declarations go here

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern float glob_temperature;
extern float glob_humidity;

extern String wifi_ssid;
extern String wifi_password;
extern String CORE_IOT_TOKEN;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

#endif // GLOBAL_H