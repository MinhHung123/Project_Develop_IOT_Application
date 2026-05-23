#ifndef TASK_LED_BLINK_H
#define TASK_LED_BLINK_H

#include <Arduino.h>

/*
 * The LED blink behavior changes according to the latest temperature sample exposed by sensor_read() (mutex-protected, see ADR-03 in INSTRUCTIONS.md).
 * The task is woken up by xBinarySemaphoreLed, which is given by the
 * DHT20 producer (task_temp_humid) on every fresh reading (ADR-04 Cach 1).
 *
 * Behaviors (3 levels, satisfy Task 1 requirement of >= 3 distinct patterns):
 *   - temp <  TEMP_THRESHOLD_LOW   : slow blink (cool, periodic reassurance)
 *   - temp <  TEMP_THRESHOLD_HIGH  : medium blink (normal operating range)
 *   - temp >= TEMP_THRESHOLD_HIGH  : fast blink (hot, attention needed)
 *
 * If sensor_read() returns false (no valid sample yet) the LED is kept off
 * to avoid misleading the operator.
 */

#define LED_GPIO 48
#define LED_ON   HIGH
#define LED_OFF  LOW

// Temperature thresholds in degrees Celsius.
#define TEMP_THRESHOLD_LOW   25.0f
#define TEMP_THRESHOLD_HIGH  35.0f

// Blink 
#define LED_PERIOD_SLOW_MS     1000   // 0.5 Hz blink: cool range
#define LED_PERIOD_MEDIUM_MS    500   // 1.0 Hz blink: normal range
#define LED_PERIOD_FAST_MS      150   // ~3.3 Hz blink: hot range

// Target total blinking duration per wake-up, in milliseconds.
// Chosen to match the DHT20 producer period (2000 ms) so that the LED
// keeps blinking continuously instead of pausing between wake-ups.
#define LED_BURST_DURATION_MS  2000

void task_led_blink(void *pvParameters);

#endif // TASK_LED_BLINK_H