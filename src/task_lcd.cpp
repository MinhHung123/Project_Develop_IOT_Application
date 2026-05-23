#include "task_lcd.h"
#include "global.h"
#include "freertos/semphr.h"
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <stdio.h>

static LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

/* -----------------------------------------------------------------------
 * Status classification
 * ----------------------------------------------------------------------- */

typedef enum {
    STATUS_NORMAL,
    STATUS_WARNING,
    STATUS_CRITICAL
} env_status_t;

static env_status_t classify_status(float t, float h) {
    /* CRITICAL: any value outside safe physical range */
    if (t < TEMP_CRITICAL_LOW  || t > TEMP_CRITICAL_HIGH ||
        h < HUMI_CRITICAL_LOW  || h > HUMI_CRITICAL_HIGH) {
        return STATUS_CRITICAL;
    }
    /* NORMAL: both temp and humi inside comfort band */
    if (t >= TEMP_NORMAL_LOW && t <= TEMP_NORMAL_HIGH &&
        h >= HUMI_NORMAL_LOW && h <= HUMI_NORMAL_HIGH) {
        return STATUS_NORMAL;
    }
    /* WARNING: transition band between NORMAL and CRITICAL */
    return STATUS_WARNING;
}

static const char *status_to_str(env_status_t s) {
    switch (s) {
        case STATUS_NORMAL:   return "STATUS: NORMAL  ";  /* 16 chars */
        case STATUS_WARNING:  return "STATUS: WARNING ";  /* 16 chars */
        case STATUS_CRITICAL: return "STATUS: CRITICAL";  /* 16 chars */
        default:              return "STATUS: ???     ";  /* 16 chars */
    }
}

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */

/* Write a 16-char line only if it changed (partial update, avoids flicker). */
static void lcd_write_line(uint8_t row, const char *new_str, char *last_str) {
    if (strncmp(new_str, last_str, LCD_COLS) != 0) {
        lcd.setCursor(0, row);
        lcd.print(new_str);
        strncpy(last_str, new_str, LCD_COLS);
        last_str[LCD_COLS] = '\0';
    }
}

/* -----------------------------------------------------------------------
 * @brief Render Screen 1: EMA-filtered temp + humi, STATUS classification.
 *        Uses %-16.16s to guarantee exactly LCD_COLS chars on every path.
 * ----------------------------------------------------------------------- */
static void render_screen_sensor(char *last_line0, char *last_line1) {
    static bool  ema_initialized = false;
    static float ema_temp        = 0.0f;
    static float ema_humi        = 0.0f;

    SensorData data;

    if (!sensor_read(&data) || !data.valid) {
        /* Literal strings are exactly 16 chars — no snprintf needed */
        lcd_write_line(0, "T:--.- H:--.-%  ", last_line0); /* 16 chars */
        lcd_write_line(1, "STATUS: SENS ERR", last_line1); /* 16 chars */
        Serial.println("[task_lcd] sensor error");
        return;
    }

    /* EMA filter — initialise from first sample to avoid cold-start spike */
    if (!ema_initialized) {
        ema_temp        = data.temp;
        ema_humi        = data.humi;
        ema_initialized = true;
    } else {
        ema_temp = EMA_ALPHA * data.temp + (1.0f - EMA_ALPHA) * ema_temp;
        ema_humi = EMA_ALPHA * data.humi + (1.0f - EMA_ALPHA) * ema_humi;
    }

    /* Line 0: build in tmp then clamp to exactly 16 chars with %-16.16s */
    char tmp[24];
    char line0_new[17];
    snprintf(tmp, sizeof(tmp), "T:%.1f H:%.1f%%", ema_temp, ema_humi);
    snprintf(line0_new, sizeof(line0_new), "%-16.16s", tmp);

    /* Line 1: status_to_str already returns exactly 16 chars */
    env_status_t st = classify_status(ema_temp, ema_humi);

    lcd_write_line(0, line0_new, last_line0);
    lcd_write_line(1, status_to_str(st), last_line1);

    Serial.print("[task_lcd] T=");
    Serial.print(ema_temp, 1);
    Serial.print(" H=");
    Serial.print(ema_humi, 1);
    Serial.print("  ");
    Serial.println(status_to_str(st));
}

/* -----------------------------------------------------------------------
 * @brief Render Screen 2: device state stub — servo door + fan speed.
 *        LED1/LED2 deferred until lighting module is integrated.
 * ----------------------------------------------------------------------- */
static void render_screen_devices(char *last_line0, char *last_line1) {
    /* TODO: add LED1/LED2 status when lighting module is integrated */

    /* pos_servo: 0 = CLOSED, 90 = OPEN (toggled by CoreIOT RPC setDoor) */
    const char *door_str = (pos_servo > 45) ? "OPN" : "CLS";
    /* fan_speed: 0-100 percent; 0 = OFF */
    const char *fan_str  = (fan_speed > 0)  ? "ON " : "OFF";

    char tmp[24];
    char line0_new[17];
    snprintf(tmp, sizeof(tmp), "DOOR:%-3s FAN:%-3s", door_str, fan_str);
    snprintf(line0_new, sizeof(line0_new), "%-16.16s", tmp);

    lcd_write_line(0, line0_new, last_line0);
    lcd_write_line(1, "Coming soon...  ", last_line1);  /* 16 chars */
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/**
 * @brief Initialize LCD hardware. Call from setup() after Wire.begin().
 */
void lcd_init(void) {
    lcd.begin();
    lcd.backlight();
    lcd.noCursor();
    lcd.noBlink();
}

/**
 * @brief FreeRTOS task: consumer of xBinarySemaphoreLcd.
 *        Renders sensor data and device state on 16x2 I2C LCD.
 */
void task_lcd_display(void *pvParameters) {
    (void) pvParameters;

    static uint8_t      wake_count     = 0;
    static lcd_screen_t current_screen = SCREEN_SENSOR;
    static char         last_line0[17] = {0};
    static char         last_line1[17] = {0};

    /* Boot splash */
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Smart Home v1.0");
    lcd.setCursor(0, 1); lcd.print("Booting...");
    vTaskDelay(pdMS_TO_TICKS(LCD_BOOT_SPLASH_MS));
    lcd.clear();

    /* Eager first render: avoid blank LCD while waiting for first sensor signal */
    render_screen_sensor(last_line0, last_line1);

    Serial.println("[task_lcd] started");

    for (;;) {
        if (xSemaphoreTake(xBinarySemaphoreLcd, portMAX_DELAY) == pdTRUE) {
            wake_count++;
            if (wake_count >= SCREEN_SWITCH_CYCLES) {
                wake_count     = 0;
                current_screen = (lcd_screen_t)((current_screen + 1) % SCREEN_COUNT);
                lcd.clear();
                memset(last_line0, 0, sizeof(last_line0));
                memset(last_line1, 0, sizeof(last_line1));
            }

            switch (current_screen) {
                case SCREEN_SENSOR:
                    render_screen_sensor(last_line0, last_line1);
                    break;
                case SCREEN_DEVICES:
                    render_screen_devices(last_line0, last_line1);
                    break;
                default:
                    break;
            }
        }
    }
}
