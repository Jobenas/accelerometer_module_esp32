#ifndef ACCELEROMETER_INTERFACE_H
#define ACCELEROMETER_INTERFACE_H

#include "accelerometer_config.h"

// High-level accelerometer interface wrapper
class AccelerometerInterface {
private:
    bool is_initialized;
    AccelData last_reading;
    unsigned long last_read_time;
    
public:
    AccelerometerInterface();
    
    // Initialization and control
    bool begin();
    void end();
    
    // Data reading
    bool readData(AccelData& data);
    AccelData getLastReading() const { return last_reading; }
    unsigned long getLastReadTime() const { return last_read_time; }
    
    // Status and information
    bool isInitialized() const { return is_initialized; }
    const char* getSensorName() const;
    void printSensorInfo() const;
    
    // Utility functions
    void printBuildInfo() const;
};

// Global accelerometer interface instance
extern AccelerometerInterface accelerometer;

#endif // ACCELEROMETER_INTERFACE_H
