#include "task_coreIOT.h"

const char* mqtt_server = "app.coreiot.io";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void publishSensor(float temperature, float humidity, int fan_speed);

static unsigned long lastTelemetryPublishMs = 0;

static void publishCurrentTelemetry() {
    publishSensor(glob_temperature, glob_humidity, fan_speed);
    lastTelemetryPublishMs = millis();
}

void reconnect(){
    String clientId = "ESP32S3-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    while(!client.connected()){
        Serial.print("Attempting MQTT connection...");
        if (client.connect(clientId.c_str(), CORE_IOT_TOKEN.c_str(), "")) {
            Serial.println("connected to server");
            client.subscribe("v1/devices/me/rpc/request/+");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void publishSensor(float temperature, float humidity, int fan_speed){
    const char* topic = "v1/devices/me/telemetry";
    JsonDocument doc;
    doc["fan_speed"] = fan_speed;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;

    char buffer[256];
    size_t n = serializeJson(doc, buffer);
    if(client.publish(topic, buffer, n)){
        Serial.println("Data published " + String(buffer));
    } else {
        Serial.println("Failed to publish data");
    }
}

void updateAndPulish(String &method, int paramVal){
    if(method == "setFan"){
        fan_speed = max(0, min(100, paramVal)); // Ensure fan speed is between 0 and 100
        Serial.println("Fan speed set to " + String(fan_speed) + "%");
        int fan_val = map(fan_speed, 0, 100, 0, 255); // Map to PWM range
        analogWrite(FAN_PIN, fan_val);
    }

    if(method == "setDoor"){
        pos_servo = pos_servo > 0 ? 0 : 90; 
        myServo.write(pos_servo);
        if(pos_servo == 0){
            Serial.println("Closed the door");
        } else {
            Serial.println("Opened the door");
        }
    }

    publishSensor(glob_temperature, glob_humidity, fan_speed);
}

void callback(char* topic, byte* payload, unsigned int length){
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0'; // Null-terminate the message

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(message);

    JsonDocument doc;
    deserializeJson(doc, message);

    String method = doc["method"] | "";
    int param_val = doc["params"] | 0;

    // Handle RPC requests here
    updateAndPulish(method, param_val);
}

void TaskCoreIOT(void *pvParameters) {
    myServo.attach(DOOR_SERVO_PIN); // Attach servo to configured door servo pin
    myServo.write(0); // Initialize servo to 0 degrees
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    while(1){
        if (WiFi.status() != WL_CONNECTED) {
            if (uxSemaphoreGetCount(xBinarySemaphoreInternet) != 0) {
                while (xSemaphoreTake(xBinarySemaphoreInternet, 0) == pdTRUE) {
                    // Clear the semaphore so MQTT pauses until WiFi returns.
                }
            }
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (uxSemaphoreGetCount(xBinarySemaphoreInternet) == 0) {
            xSemaphoreGive(xBinarySemaphoreInternet);
        }

        if(!client.connected()){
            reconnect();
        }
        client.loop();

        bool shouldPublish = false;
        if (xSemaphoreTake(xBinarySemaphoreFanUpdate, 0) == pdTRUE) {
            shouldPublish = true;
        }

        if (millis() - lastTelemetryPublishMs >= 2000) {
            shouldPublish = true;
        }

        if (shouldPublish) {
            publishCurrentTelemetry();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}