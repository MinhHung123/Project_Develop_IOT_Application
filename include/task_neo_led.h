#ifndef TASK_NEO_LED_H
#define TASK_NEO_LED_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "global.h"

#define NEOPIXEL_PIN 45
#define LED_COUNT 1

void TaskNEOLED(void *pvParameters);

#endif // TASK_NEO_LED_H
