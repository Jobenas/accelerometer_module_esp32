#ifndef ADXL355_H
#define ADXL355_H

#include <Arduino.h>
#include <SPI.h>
#include "config.h"

class ADXL355 {
private:
  bool initialized;
  
public:
  ADXL355();
  
  // Initialization
  bool begin();
  void end();
  
  // Low-level register access
  void writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);
  
  // Data reading
  void readXYZ(int32_t &x, int32_t &y, int32_t &z);
  void readAcceleration(float &x_g, float &y_g, float &z_g);
  
  // Device identification
  bool checkDeviceID();
  void printDeviceInfo();
  
  // Status
  bool isInitialized() const { return initialized; }
  float getScaleFactor() const;
};

#endif // ADXL355_H
