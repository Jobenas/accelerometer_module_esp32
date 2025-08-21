#include "modbus_interface.h"
#include "config.h"

ModbusInterface::ModbusInterface() {
  initialized = false;
  last_update_time = 0;
}

ModbusInterface::~ModbusInterface() {
  stop();
}

bool ModbusInterface::begin() {
  if (initialized) return true;
  
  #if ENABLE_DEBUG_OUTPUT
  Serial.println("[ModbusInterface] Initializing custom Modbus RTU...");
  #endif
  
  // Initialize our custom Modbus RTU implementation with explicit parameters
  if (!modbusRTU.begin(MODBUS_SLAVE_ID, MODBUS_BAUDRATE, MODBUS_RX_PIN, MODBUS_TX_PIN, MODBUS_DE_RE_PIN)) {
    #if ENABLE_DEBUG_OUTPUT
    Serial.println("[ModbusInterface] Failed to initialize Modbus RTU!");
    #endif
    return false;
  }
  
  initialized = true;
  last_update_time = millis();
  
  #if ENABLE_DEBUG_OUTPUT
  Serial.println("[ModbusInterface] Custom Modbus RTU initialized successfully");
  Serial.println("[ModbusInterface] Configuration:");
  Serial.printf("  Slave ID: %d\n", MODBUS_SLAVE_ID);
  Serial.printf("  Baudrate: %d\n", MODBUS_BAUDRATE);
  Serial.printf("  RX Pin: %d\n", MODBUS_RX_PIN);
  Serial.printf("  TX Pin: %d\n", MODBUS_TX_PIN);
  Serial.printf("  DE/RE Pin: %d\n", MODBUS_DE_RE_PIN);
  Serial.println("[ModbusInterface] Ready to receive Modbus requests...");
  #endif
  
  return true;
}

void ModbusInterface::update() {
  if (!initialized) return;
  
  // Update the custom Modbus RTU implementation
  modbusRTU.update();
  
  last_update_time = millis();
  
  // Debug output every 10 seconds
  static unsigned long last_debug = 0;
  if (millis() - last_debug > 10000) {
    #if ENABLE_DEBUG_OUTPUT
    Serial.println("[ModbusInterface] Modbus task running, calling RTU update");
    #endif
    last_debug = millis();
  }
}

void ModbusInterface::stop() {
  if (initialized) {
    modbusRTU.stop();
    initialized = false;
    
    #if ENABLE_DEBUG_OUTPUT
    Serial.println("[ModbusInterface] Stopped");
    #endif
  }
}

void ModbusInterface::printRegisterMap() {
  modbusRTU.printRegisterMap();
}

void ModbusInterface::printStats() {
  modbusRTU.printStats();
}
