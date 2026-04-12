#include "task_temp_humid.h"

#define SDA_PIN 11
#define SCL_PIN 12

DHT20 dht20; // Create DHT20 sensor object

void task_temp_humid(void *pvParameters) {
    dht20.begin(); // Initialize the DHT20 sensor
    while(1){
        dht20.read();
        float temp = dht20.getTemperature();
        float humid = dht20.getHumidity();

        if(isnan(temp) || isnan(humid)) {
            Serial.println("Failed to read from DHT20 sensor!");
            temp = humid = -1.0; // Set to -1 to indicate error
        } 

        glob_temperature = temp;
        glob_humidity = humid;
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.print(" °C, Humidity: ");
        Serial.print(humid);
        Serial.println(" %");
        vTaskDelay(2000); // Delay for 2 seconds before next reading
    }
}