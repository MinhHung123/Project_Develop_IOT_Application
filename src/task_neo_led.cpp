#include "task_neo_led.h"

void TaskNEOLED(void *pvParameters) {
    Adafruit_NeoPixel strip(LED_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show(); // Initialize all pixels to 'off'

    while(1) {
        xSemaphoreTake(xBinarySemaphoreTemp_Humi, portMAX_DELAY); // Wait for temperature and humidity data to be available
        if(glob_humidity < 30){
            strip.setPixelColor(0, strip.Color(100, 0, 0)); // Red
            strip.show();
        } else if(glob_humidity >= 30 && glob_humidity < 60){
            strip.setPixelColor(0, strip.Color(100, 100, 0)); // Yellow
            strip.show();
        } else {
            strip.setPixelColor(0, strip.Color(0, 100, 0)); // Green
            strip.show();
        }
    }
}
