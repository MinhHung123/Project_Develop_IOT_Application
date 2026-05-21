#ifndef GLOBAL_H
#define GLOBAL_H

// Global variables and function declarations go here

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <ESP32Servo.h>

extern float glob_temperature;
extern float glob_humidity;
extern int fan_speed;
extern Servo myServo;
extern int pos_servo;

extern String wifi_ssid;
extern String wifi_password;
extern String CORE_IOT_TOKEN;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern SemaphoreHandle_t xBinarySemaphoreTemp_Humi;
extern SemaphoreHandle_t xBinarySemaphoreFanUpdate;

extern volatile bool led_manual_mode;
extern volatile uint8_t led_manual_r;
extern volatile uint8_t led_manual_g;
extern volatile uint8_t led_manual_b;

#endif // GLOBAL_H