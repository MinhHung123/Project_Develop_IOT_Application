#include "button.h"
#include "task_coreIOT.h"

void task_button(void *pvParameters){
    (void)pvParameters;

    pinMode(BUTTON_PIN_1, INPUT_PULLUP);
    pinMode(BUTTON_PIN_2, INPUT_PULLUP);
    pinMode(FAN_PIN, OUTPUT);

    const TickType_t pollDelay = pdMS_TO_TICKS(5);
    const uint32_t debounceMs = 30;

    // Debounce state for button 1
    int lastRawBtn1 = digitalRead(BUTTON_PIN_1);
    int stableBtn1 = lastRawBtn1;
    uint32_t lastChangeBtn1Ms = millis();

    while(1){
        int rawBtn1 = digitalRead(BUTTON_PIN_1);
        int rawBtn2 = digitalRead(BUTTON_PIN_2);

        // Placeholder for button 2 behavior in future.
        (void)rawBtn2;

        if(rawBtn1 != lastRawBtn1){
            lastRawBtn1 = rawBtn1;
            lastChangeBtn1Ms = millis();
        }

        if((millis() - lastChangeBtn1Ms) >= debounceMs && stableBtn1 != rawBtn1){
            stableBtn1 = rawBtn1;

            // Active-low press event on stable transition.
            if(stableBtn1 == LOW){
                fan_speed = (fan_speed > 0) ? 0 : 100;
                int fan_val = map(fan_speed, 0, 100, 0, 255);
                analogWrite(FAN_PIN, fan_val);
                Serial.println(String("Button 1 pressed -> Fan ") + (fan_speed > 0 ? "ON" : "OFF"));
            }
        }

        vTaskDelay(pollDelay);
    }
}