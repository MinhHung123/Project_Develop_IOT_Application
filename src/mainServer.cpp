#include "mainServer.h"

void setupSTAMode() {
    // Set WiFi mode to Station (STA) mode
    WiFi.mode(WIFI_STA);
    
    // Disconnect from any previous connection
    WiFi.disconnect(true);  // true = turn off WiFi radio
    delay(100);
    
    // Start WiFi connection
    Serial.println("\n\nStarting WiFi connection...");
    Serial.print("Connecting to: ");
    Serial.println(wifi_ssid);
    
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    
    // Wait for connection with timeout
    int attempts = 0;
    const int max_attempts = 20;  // ~10 seconds timeout (500ms per attempt)
    
    while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected successfully!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal strength (RSSI): ");
        Serial.println(WiFi.RSSI());
        xSemaphoreGive(xBinarySemaphoreInternet); // Signal that internet is available
    } else {
        Serial.println("Failed to connect to WiFi!");
        Serial.print("WiFi Status: ");
        Serial.println(WiFi.status());
    }
}