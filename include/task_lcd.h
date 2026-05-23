#ifndef TASK_LCD_H
#define TASK_LCD_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Screen indices for multi-screen rotation */
typedef enum {
    SCREEN_SENSOR  = 0,   /* temp + humi + status (active) */
    SCREEN_DEVICES = 1,   /* device states (stub) */
    SCREEN_COUNT
} lcd_screen_t;

/* --- Threshold macros (Screen 1 status logic) --- */
#define TEMP_NORMAL_LOW    20.0f
#define TEMP_NORMAL_HIGH   30.0f
#define TEMP_CRITICAL_LOW  15.0f
#define TEMP_CRITICAL_HIGH 35.0f
#define HUMI_NORMAL_LOW    30.0f
#define HUMI_NORMAL_HIGH   70.0f
#define HUMI_WARNING_HIGH  85.0f
#define HUMI_CRITICAL_LOW  20.0f
#define HUMI_CRITICAL_HIGH 85.0f

/* EMA smoothing factor */
#define EMA_ALPHA          0.3f

/* Wake cycles before screen switch (~4 s at 2 s/cycle) */
#define SCREEN_SWITCH_CYCLES  2U

/* Boot splash duration ms */
#define LCD_BOOT_SPLASH_MS    1500U

/* LCD hardware */
#define LCD_I2C_ADDR  0x21
#define LCD_COLS      16
#define LCD_ROWS      2


void lcd_init(void);
void task_lcd_display(void *pvParameters);

#endif /* TASK_LCD_H */