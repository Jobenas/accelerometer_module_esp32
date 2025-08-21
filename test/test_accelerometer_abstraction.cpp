// Test program to verify accelerometer abstraction works
// Compile with either USE_ADXL355 or USE_MPU6050 defined

#include <Arduino.h>
#include "accelerometer_interface.h"

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== Accelerometer Abstraction Test ===");
    
    // Initialize accelerometer
    if (!accelerometer.begin()) {
        Serial.println("ERROR: Failed to initialize accelerometer!");
        while (1) {
            delay(1000);
            Serial.println("Initialization failed - check connections and sensor selection");
        }
    }
    
    Serial.println("Accelerometer initialized successfully!");
    Serial.println("Starting data reading test...");
}

void loop() {
    AccelData data;
    
    if (accelerometer.readData(data) && data.valid) {
        // Print the data
        Serial.printf("[%s] X: %7.3f g, Y: %7.3f g, Z: %7.3f g\n", 
                     accelerometer.getSensorName(), data.x, data.y, data.z);
        
        // Calculate magnitude
        float magnitude = sqrt(data.x * data.x + data.y * data.y + data.z * data.z);
        Serial.printf("Magnitude: %.3f g\n", magnitude);
        
        // Show timestamp
        Serial.printf("Last read: %lu ms ago\n", millis() - accelerometer.getLastReadTime());
        
    } else {
        Serial.println("Failed to read accelerometer data!");
    }
    
    Serial.println("---");
    delay(1000); // Read every second
}
