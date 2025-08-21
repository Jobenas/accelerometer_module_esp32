#include "accelerometer_interface.h"
#include <Arduino.h>

// Global accelerometer interface instance
AccelerometerInterface accelerometer;

AccelerometerInterface::AccelerometerInterface() 
    : is_initialized(false), last_read_time(0) {
    last_reading.x = 0.0f;
    last_reading.y = 0.0f;
    last_reading.z = 0.0f;
    last_reading.valid = false;
}

bool AccelerometerInterface::begin() {
    Serial.println("=== Accelerometer Interface ===");
    printBuildInfo();
    
    is_initialized = accel_init();
    
    if (is_initialized) {
        Serial.printf("Successfully initialized %s\n", accel_get_name());
        printSensorInfo();
    } else {
        Serial.printf("Failed to initialize %s\n", accel_get_name());
    }
    
    return is_initialized;
}

void AccelerometerInterface::end() {
    if (is_initialized) {
        accel_deinit();
        is_initialized = false;
        Serial.printf("%s deinitialized\n", accel_get_name());
    }
}

bool AccelerometerInterface::readData(AccelData& data) {
    if (!is_initialized) {
        data.valid = false;
        return false;
    }
    
    bool success = accel_read(data);
    
    if (success) {
        last_reading = data;
        last_read_time = millis();
    }
    
    return success;
}

const char* AccelerometerInterface::getSensorName() const {
    return accel_get_name();
}

void AccelerometerInterface::printSensorInfo() const {
    accel_print_info();
}

void AccelerometerInterface::printBuildInfo() const {
    Serial.println("=== Build Configuration ===");
    
    #ifdef USE_ADXL355
        Serial.println("Accelerometer: ADXL355 (SPI)");
        Serial.println("Precision: High (20-bit)");
        Serial.println("Power: Ultra-low noise");
    #endif
    
    #ifdef USE_MPU6050
        Serial.println("Accelerometer: MPU6050 (I2C)");
        Serial.println("Precision: Standard (16-bit)");
        Serial.println("Features: 6-axis (accel + gyro)");
    #endif
    
    Serial.printf("Compiled: %s %s\n", __DATE__, __TIME__);
    Serial.println("============================");
}
