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


struct SensorData{
    float temp;
    float humi;
    uint32_t timestamp_ms;
    bool valid;
};

extern SemaphoreHandle_t xMutexSensorData;
extern SemaphoreHandle_t xBinarySemaphoreLed;
extern SemaphoreHandle_t xBinarySemaphoreLcd;
extern SemaphoreHandle_t xBinarySemaphoreTinyML;

extern volatile int   glob_ml_label;
extern volatile float glob_ml_confidence;

/*
 - Atomically read the latest sensor sample.
 - Pointer to caller-provided SensorData; populated on success.
 - return true if a valid sample was copied, false if no sample yet or mutex could not be acquired.
 */
bool sensor_read(SensorData* out);

/*
 - Atomically store a new sensor sample.
 - In the sample to store. The valid flag must be set by the caller.
 - return true on success, false if mutex could not be acquired.
 */
bool sensor_write(const SensorData& in);

#endif // GLOBAL_H