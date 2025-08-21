#include "accelerometer_config.h"

#ifdef USE_ADXL355

#include <Arduino.h>
#include <SPI.h>
#include "adxl355.h"

// Global ADXL355 instance
static ADXL355 adxl355_sensor;

bool accel_init() {
    Serial.println("Initializing ADXL355...");
    
    if (!adxl355_sensor.begin()) {
        Serial.println("Failed to initialize ADXL355!");
        return false;
    }
    
    // Print device information
    adxl355_sensor.printDeviceInfo();
    
    Serial.println("ADXL355 initialized successfully");
    return true;
}

bool accel_read(AccelData& data) {
    if (!adxl355_sensor.isInitialized()) {
        data.valid = false;
        return false;
    }
    
    // Read acceleration data in g-force units
    adxl355_sensor.readAcceleration(data.x, data.y, data.z);
    data.valid = true;
    
    return true;
}

void accel_deinit() {
    Serial.println("Deinitializing ADXL355...");
    adxl355_sensor.end();
}

const char* accel_get_name() {
    return "ADXL355";
}

void accel_print_info() {
    Serial.println("=== ADXL355 Information ===");
    Serial.println("Interface: SPI");
    Serial.println("Resolution: 20-bit");
    Serial.println("Range: ±2g/±4g/±8g");
    Serial.println("Noise: Ultra-low");
    Serial.printf("Scale Factor: %.6f g/LSB\n", adxl355_sensor.getScaleFactor());
    Serial.println("============================");
}

#endif // USE_ADXL355
