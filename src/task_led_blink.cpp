#include "task_led_blink.h"
#include "global.h"


static uint32_t pick_half_period_ms(float temp_c) {
    if (temp_c < TEMP_THRESHOLD_LOW)  return LED_PERIOD_SLOW_MS;
    if (temp_c < TEMP_THRESHOLD_HIGH) return LED_PERIOD_MEDIUM_MS;
    return LED_PERIOD_FAST_MS;

}

/*
 * Compute how many full blink cycles to perform per wake-up so that the total burst duration approximates LED_BURST_DURATION_MS.
 * Producer (DHT20) fires every ~2000 ms. To keep the LED visibly blinking across the entire producer cycle, we run several blink cycles per wake-up.
 * Returns at least 1 to guarantee at least one visible blink.
 */
static uint32_t compute_blink_count(uint32_t half_period_ms) {
    // One full cycle = 2 * half_period (LED on + LED off).
    uint32_t full_cycle_ms = 2 * half_period_ms;
    uint32_t n = LED_BURST_DURATION_MS / full_cycle_ms;
    return (n == 0) ? 1 : n;
}

void task_led_blink(void *pvParameters) {
    (void) pvParameters;

    // Hardware init: configure the LED GPIO and force it OFF.
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LED_OFF);

    Serial.println("[task_led_blink] started");

    while (1) {
        xSemaphoreTake(xBinarySemaphoreLed, portMAX_DELAY);

        SensorData sample;
        if (!sensor_read(&sample)) {
            digitalWrite(LED_GPIO, LED_OFF);
            continue;
        }

        uint32_t half_period_ms = pick_half_period_ms(sample.temp);
        uint32_t n_cycles       = compute_blink_count(half_period_ms);

        Serial.print("[task_led_blink] temp=");
        Serial.print(sample.temp, 1);
        Serial.print("C band_half=");
        Serial.print(half_period_ms);
        Serial.print("ms cycles=");
        Serial.println(n_cycles);

        for (uint32_t i = 0; i < n_cycles; i++) {
            digitalWrite(LED_GPIO, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(half_period_ms));
            digitalWrite(LED_GPIO, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(half_period_ms));
        }
    }
}