#ifndef MODBUS_INTERFACE_H
#define MODBUS_INTERFACE_H

#include <Arduino.h>
#include "modbus_rtu_custom.h"
#include "analytics.h"

// This is now a wrapper around our custom Modbus RTU implementation
class ModbusInterface {
private:
  ModbusRTUCustom* modbus_custom;
  bool initialized;
  unsigned long last_update_time;
  
public:
  ModbusInterface();
  ~ModbusInterface();
  
  // Initialization and control
  bool begin();
  void update();
  void stop();
  
  // Status
  bool isInitialized() const { return initialized; }
  unsigned long getLastUpdateTime() const { return last_update_time; }
  
  // Register access functions for debugging
  void printRegisterMap();
  void printStats();
};

// Global modbus interface instance
extern ModbusInterface modbusInterface;

#endif // MODBUS_INTERFACE_H
