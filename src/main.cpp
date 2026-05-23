#include <Arduino.h>
#include "mainServer.h"
#include "global.h"
#include "button.h"
#include "task_led_blink.h"
#include "task_neo_led.h"
#include "task_temp_humid.h"
#include "task_coreIOT.h"
#include "task_lcd.h"
#include "task_tinyml.h"

#define SDA_PIN 11
#define SCL_PIN 12

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN); // Initialize I2C communication
  lcd_init();
  setupSTAMode(); // Set up WiFi connection


  xTaskCreate(task_led_blink, "LED Blink Task", 2048, NULL, 1, NULL);
  xTaskCreate(TaskCoreIOT, "CoreIOT Task", 4096, NULL, 1, NULL);
  xTaskCreate(task_temp_humid, "TempHumid Task", 2048, NULL, 2, NULL);
  xTaskCreate(task_lcd_display, "LCD Task", 4096, NULL, 1, NULL);
  xTaskCreate(task_tinyml_inference, "TinyML", 8192, NULL, 1, NULL);

  xTaskCreate(task_button, "Button Task", 2048, NULL, 2, NULL);
  xTaskCreate(TaskNEOLED, "NEOLED Task", 2048, NULL, 1, NULL);
  xTaskCreate(TaskWebServer, "WebServer Task", 6144, NULL, 1, NULL);
}

void loop() {
  // Serial.println("Hello Custom Board");
  // delay(1000);
}