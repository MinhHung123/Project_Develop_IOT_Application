#ifndef TASK_COREIOT_H
#define TASK_COREIOT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include "global.h"

#define FAN_PIN 10
#define DOOR_SERVO_PIN 5

void TaskCoreIOT(void *pvParameters);

#endif // TASK_COREIOT_H