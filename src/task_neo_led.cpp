#include "task_neo_led.h"

const LedColor NeoLedModule::kRed = {100, 0, 0};
const LedColor NeoLedModule::kYellow = {100, 100, 0};
const LedColor NeoLedModule::kGreen = {0, 100, 0};
const LedColor NeoLedModule::kCyan = {0, 100, 100};
const LedColor NeoLedModule::kBlue = {0, 0, 100};
const LedColor NeoLedModule::kMagenta = {100, 0, 100};
const LedColor NeoLedModule::kOrange = {100, 50, 0};
const LedColor NeoLedModule::kPurple = {50, 0, 100};
const LedColor NeoLedModule::kSkyBlue = {0, 50, 100};
const LedColor NeoLedModule::kWhite = {100, 100, 100};
const LedColor NeoLedModule::kDimGray = {30, 30, 30};
const LedColor NeoLedModule::kSoftRed = {100, 20, 20};
const LedColor NeoLedModule::kSoftGreen = {20, 100, 20};
const LedColor NeoLedModule::kSoftBlue = {20, 20, 100};

const LedColor NeoLedModule::kPalette[] = {
    NeoLedModule::kRed,
    NeoLedModule::kYellow,
    NeoLedModule::kGreen,
    NeoLedModule::kCyan,
    NeoLedModule::kBlue,
    NeoLedModule::kMagenta,
    NeoLedModule::kOrange,
    NeoLedModule::kPurple,
    NeoLedModule::kSkyBlue,
    NeoLedModule::kWhite,
    NeoLedModule::kDimGray,
    NeoLedModule::kSoftRed,
    NeoLedModule::kSoftGreen,
    NeoLedModule::kSoftBlue,
    {100, 30, 80},
    {80, 100, 30},
    {30, 80, 100},
    {100, 80, 30},
    {80, 30, 100},
    {30, 100, 80},
    {60, 10, 10},
    {10, 60, 10},
    {10, 10, 60},
    {90, 40, 40},
    {40, 90, 40},
    {40, 40, 90},
};

const uint8_t NeoLedModule::kPaletteSize = sizeof(NeoLedModule::kPalette) / sizeof(NeoLedModule::kPalette[0]);

void TaskNEOLED(void *pvParameters) {
    Adafruit_NeoPixel strip(LED_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show(); // Initialize all pixels to 'off'

    while(1) {
        xSemaphoreTake(xBinarySemaphoreTemp_Humi, portMAX_DELAY); // Wait for temperature and humidity data to be available
        if(glob_humidity < 30){
            strip.setPixelColor(0, strip.Color(
                NeoLedModule::kPalette[0].red,
                NeoLedModule::kPalette[0].green,
                NeoLedModule::kPalette[0].blue)); // Red
            strip.show();
        } else if(glob_humidity >= 30 && glob_humidity < 60){
            strip.setPixelColor(0, strip.Color(
                NeoLedModule::kPalette[1].red,
                NeoLedModule::kPalette[1].green,
                NeoLedModule::kPalette[1].blue)); // Yellow
            strip.show();
        } else {
            strip.setPixelColor(0, strip.Color(
                NeoLedModule::kPalette[2].red,
                NeoLedModule::kPalette[2].green,
                NeoLedModule::kPalette[2].blue)); // Green
            strip.show();
        }
    }
}
