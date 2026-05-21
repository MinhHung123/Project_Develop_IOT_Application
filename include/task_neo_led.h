#ifndef TASK_NEO_LED_H
#define TASK_NEO_LED_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "global.h"

#define NEOPIXEL_PIN 45
#define LED_COUNT 1

struct LedColor {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

namespace NeoLedModule {
	extern const LedColor kRed;
	extern const LedColor kYellow;
	extern const LedColor kGreen;
	extern const LedColor kCyan;
	extern const LedColor kBlue;
	extern const LedColor kMagenta;
	extern const LedColor kOrange;
	extern const LedColor kPurple;
	extern const LedColor kSkyBlue;
	extern const LedColor kWhite;
	extern const LedColor kDimGray;
	extern const LedColor kSoftRed;
	extern const LedColor kSoftGreen;
	extern const LedColor kSoftBlue;

	extern const LedColor kPalette[];
	extern const uint8_t kPaletteSize;
}

void TaskNEOLED(void *pvParameters);

#endif // TASK_NEO_LED_H
