#include <Arduino.h>
#include "mainServer.h"
#include "global.h"
#include "task_neo_led.h"
#include "task_temp_humid.h"

#define SDA_PIN 11
#define SCL_PIN 12

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN); // Initialize I2C communication
  setupSTAMode(); // Set up WiFi connection

  xTaskCreate(task_temp_humid, "TempHumid Task", 2048, NULL, 1, NULL);
  xTaskCreate(TaskNEOLED, "NEOLED Task", 2048, NULL, 1, NULL);
}

void loop() {
  // Serial.println("Hello Custom Board");
  // delay(1000);
}