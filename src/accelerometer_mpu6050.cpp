#include "accelerometer_config.h"

#ifdef USE_MPU6050

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Global MPU6050 instance
static Adafruit_MPU6050 mpu6050_sensor;
static bool initialized = false;

bool accel_init() {
    Serial.println("Initializing MPU6050...");
    
    // Initialize I2C with explicit pins (SDA=21, SCL=22 are ESP32 defaults)
    Serial.println("Attempting I2C initialization on default pins (SDA=21, SCL=22)...");
    if (!Wire.begin(21, 22)) {  // SDA=21, SCL=22
        Serial.println("Failed to initialize I2C!");
        return false;
    }
    
    Serial.println("I2C initialized, scanning for MPU6050...");
    
    // I2C Scanner for debugging
    Serial.println("Scanning I2C bus...");
    int devices_found = 0;
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("I2C device found at address 0x%02X\n", address);
            devices_found++;
        }
    }
    
    if (devices_found == 0) {
        Serial.println("No I2C devices found! Check wiring.");
    } else {
        Serial.printf("Found %d I2C device(s)\n", devices_found);
        Serial.println("MPU6050 should be at address 0x68 or 0x69");
    }
    
    if (!mpu6050_sensor.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        Serial.println("Check wiring:");
        Serial.println("  VCC → 3.3V");
        Serial.println("  GND → GND"); 
        Serial.println("  SDA → GPIO 21");
        Serial.println("  SCL → GPIO 22");
        return false;
    }
    
    // Configure MPU6050 settings
    mpu6050_sensor.setAccelerometerRange(MPU6050_RANGE_2_G);      // ±2g range (same as ADXL355 default)
    mpu6050_sensor.setGyroRange(MPU6050_RANGE_250_DEG);           // ±250°/s (not used for accelerometer)
    mpu6050_sensor.setFilterBandwidth(MPU6050_BAND_21_HZ);        // 21 Hz low-pass filter
    
    initialized = true;
    Serial.println("MPU6050 initialized successfully");
    return true;
}

bool accel_read(AccelData& data) {
    if (!initialized) {
        data.valid = false;
        return false;
    }
    
    sensors_event_t accel, gyro, temp;
    
    if (!mpu6050_sensor.getEvent(&accel, &gyro, &temp)) {
        data.valid = false;
        return false;
    }
    
    // Convert from m/s² to g-force (1g = 9.80665 m/s²)
    data.x = accel.acceleration.x / 9.80665f;
    data.y = accel.acceleration.y / 9.80665f;
    data.z = accel.acceleration.z / 9.80665f;
    data.valid = true;
    
    return true;
}

void accel_deinit() {
    Serial.println("Deinitializing MPU6050...");
    initialized = false;
    // MPU6050 doesn't need explicit cleanup, but we can put it in sleep mode
    // This would require direct register access if needed
}

const char* accel_get_name() {
    return "MPU6050";
}

void accel_print_info() {
    Serial.println("=== MPU6050 Information ===");
    Serial.println("Interface: I2C");
    Serial.println("Resolution: 16-bit");
    Serial.println("Range: ±2g (configured)");
    Serial.println("Features: 6-axis (accel + gyro)");
    Serial.println("Scale Factor: 16384 LSB/g");
    Serial.println("============================");
}

#endif // USE_MPU6050
