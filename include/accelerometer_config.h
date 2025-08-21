#ifndef ACCELEROMETER_CONFIG_H
#define ACCELEROMETER_CONFIG_H

// Accelerometer selection - uncomment one
// #define USE_ADXL355
#define USE_MPU6050

// Validate configuration
#if defined(USE_ADXL355) && defined(USE_MPU6050)
    #error "Cannot define both USE_ADXL355 and USE_MPU6050"
#endif

#if !defined(USE_ADXL355) && !defined(USE_MPU6050)
    #error "Must define either USE_ADXL355 or USE_MPU6050"
#endif

// Common accelerometer interface
struct AccelData {
    float x;
    float y;
    float z;
    bool valid;
};

// Function prototypes for accelerometer interface
bool accel_init();
bool accel_read(AccelData& data);
void accel_deinit();
const char* accel_get_name();
void accel_print_info();

#endif // ACCELEROMETER_CONFIG_H
