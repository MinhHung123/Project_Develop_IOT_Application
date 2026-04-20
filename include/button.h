#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include "global.h"

#define BUTTON_PIN_1 8
#define BUTTON_PIN_2 9

void task_button(void *pvParameters);

#endif // BUTTON_H
