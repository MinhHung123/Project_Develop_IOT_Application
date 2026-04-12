#include "task_neo_led.h"

void TaskNEOLED(void *pvParameters) {
    Adafruit_NeoPixel strip(LED_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show(); // Initialize all pixels to 'off'

    while(1) {
        // Set the first LED to red
        strip.setPixelColor(0, strip.Color(255, 0, 0)); // Red color
        strip.show();
        vTaskDelay(2000); // Delay for 2 seconds

        // Set the first LED to green
        strip.setPixelColor(0, strip.Color(0, 255, 0)); // Green color
        strip.show();
        vTaskDelay(2000); // Delay for 2 seconds

        // Set the first LED to blue
        strip.setPixelColor(0, strip.Color(0, 0, 255)); // Blue color
        strip.show();
        vTaskDelay(2000); // Delay for 2 seconds
    }
}