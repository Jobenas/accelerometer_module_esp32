#include <Arduino.h>
#include "config.h"

// Test mode selection based on build flags
#ifdef SERIAL_MONITOR_TEST
  #define ACTIVE_TEST_MODE 1
#elif defined(MODBUS_TEST_MODE)
  #define ACTIVE_TEST_MODE 2
#else
  #define ACTIVE_TEST_MODE 4  // Production mode
#endif

//=============================================================================
// TEST MODE 1: RAW SERIAL MONITOR
//=============================================================================
#if ACTIVE_TEST_MODE == 1

#define RX_PIN 16
#define TX_PIN 17
#define DE_RE_PIN 4
#define BAUDRATE 9600

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("ESP32 Serial2 Raw Monitor for Modbus Debug");
  Serial.println("==========================================");
  Serial.printf("RX Pin: %d, TX Pin: %d, DE/RE Pin: %d\n", RX_PIN, TX_PIN, DE_RE_PIN);
  Serial.println("Monitoring Serial2 for incoming data...");
  Serial.println("Send Modbus request to see if ESP32 receives it.");
  Serial.println();
  
  // Initialize Serial2 with correct pins
  Serial2.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // Configure DE/RE pin for RS485 (optional for receive-only monitoring)
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW); // Receive mode
}

void loop() {
  // Check for data on Serial2
  if (Serial2.available()) {
    Serial.print("Serial2 RX: ");
    while (Serial2.available()) {
      uint8_t byte = Serial2.read();
      Serial.printf("0x%02X ", byte);
      delay(1); // Small delay to catch all bytes
    }
    Serial.println();
  }
  
  // Status indicator
  static unsigned long last_status = 0;
  if (millis() - last_status > 10000) {
    Serial.printf("Status: Listening... (uptime: %lu sec)\n", millis() / 1000);
    last_status = millis();
  }
  
  delay(10);
}

//=============================================================================
// TEST MODE 2: SIMPLE MODBUS TEST
//=============================================================================
#elif ACTIVE_TEST_MODE == 2

#include <ModbusRtu.h>

#define RX_PIN 16
#define TX_PIN 17
#define DE_RE_PIN 4
#define BAUDRATE 9600
#define SLAVE_ID 1

Modbus slave(SLAVE_ID, Serial2, DE_RE_PIN);

uint16_t au16data[16] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("ESP32 Simple Modbus RTU Slave Test");
  Serial.println("==================================");
  Serial.printf("Slave ID: %d, Baudrate: %d\n", SLAVE_ID, BAUDRATE);
  Serial.printf("Serial2 - RX: %d, TX: %d, DE/RE: %d\n", RX_PIN, TX_PIN, DE_RE_PIN);
  
  // Configure Serial2 pins
  Serial2.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  slave.begin(BAUDRATE);
  
  Serial.println("Modbus slave initialized. Registers 0-15 contain values 1-16.");
  Serial.println("Try reading holding registers 0-15 from slave ID 1.");
}

void loop() {
  slave.poll(au16data, 16);
  
  // Status and register dump every 10 seconds
  static unsigned long last_status = 0;
  if (millis() - last_status > 10000) {
    Serial.printf("Status: Running... (uptime: %lu sec)\n", millis() / 1000);
    Serial.print("Registers: ");
    for (int i = 0; i < 16; i++) {
      Serial.printf("%d ", au16data[i]);
    }
    Serial.println();
    last_status = millis();
  }
  
  delay(100);
}
//=============================================================================
// PRODUCTION MODE: FULL APPLICATION
//=============================================================================
#elif ACTIVE_TEST_MODE == 3

#define MODBUS_ENABLE_PIN 4
#define SLAVE_ADDRESS 1

uint8_t rxBuffer[50] = {'\0'};
uint16_t rxBufferIndex = 0;

// CRC16 calculation for Modbus RTU
uint16_t calculateCRC16(uint8_t *data, uint16_t length) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX, TX pins for Serial2
  pinMode(MODBUS_ENABLE_PIN, OUTPUT);
  digitalWrite(MODBUS_ENABLE_PIN, LOW); // Disable Modbus by default

  Serial.println("Testing of custom Modbus RTU slave without accelormeter first");
  Serial.println("==========================================");
  Serial.println("Waiting for Modbus requests on Serial2...");

}

void loop() {
    static unsigned long lastByteTime = 0;
    
    if (Serial2.available()) {
        // Read one byte at a time and accumulate
        while(Serial2.available()) {
            uint8_t dataByte = Serial2.read();
            lastByteTime = millis();
            
            if (rxBufferIndex < sizeof(rxBuffer) - 1) {
                rxBuffer[rxBufferIndex++] = dataByte;
                Serial.printf("Buffered byte %d: 0x%02X\n", rxBufferIndex-1, dataByte);
            } else {
                Serial.println("RX Buffer overflow, resetting index.");
                rxBufferIndex = 0;
            }
        }
    }
    
    // Process complete frame after timeout (frame gap detection)
    if (rxBufferIndex > 0 && (millis() - lastByteTime) > 10) { // 10ms timeout
        Serial.print("Complete frame received: ");
        for (uint16_t i = 0; i < rxBufferIndex; i++) {
            Serial.printf("0x%02X ", rxBuffer[i]);
        }
        Serial.println();
        
        // Process Modbus frame
        if (rxBufferIndex >= 8) { // Minimum Modbus RTU frame size
            Serial.println("Processing Modbus request...");
            uint8_t slaveAddr = rxBuffer[0];
            uint8_t funcCode = rxBuffer[1];
            
            Serial.printf("Slave Address: 0x%02X, Function Code: 0x%02X\n", slaveAddr, funcCode);
            
            if (slaveAddr == SLAVE_ADDRESS && funcCode == 0x03) {
                // Create response: [SlaveAddr][FuncCode][ByteCount][Data...][CRC_LOW][CRC_HIGH]
                uint8_t response[11]; // 3 bytes header + 6 bytes data + 2 bytes CRC
                response[0] = 0x01; // Slave address
                response[1] = 0x03; // Function code
                response[2] = 0x06; // Byte count (3 registers * 2 bytes each)
                response[3] = 0x00; // Register 0 high byte
                response[4] = 0x01; // Register 0 low byte (value = 1)
                response[5] = 0x00; // Register 1 high byte
                response[6] = 0x02; // Register 1 low byte (value = 2)
                response[7] = 0x00; // Register 2 high byte
                response[8] = 0x03; // Register 2 low byte (value = 3)
                
                // Calculate CRC16 for the response (without CRC bytes)
                uint16_t crc = calculateCRC16(response, 9);
                response[9] = crc & 0xFF;        // CRC low byte
                response[10] = (crc >> 8) & 0xFF; // CRC high byte

                Serial.print("Sending response: ");
                for (uint8_t i = 0; i < sizeof(response); i++) {
                    Serial.printf("0x%02X ", response[i]);
                }
                Serial.println();
                
                // Switch to transmit mode
                digitalWrite(MODBUS_ENABLE_PIN, HIGH);
                delayMicroseconds(100); // Small delay for RS485 switching
                
                // Send response
                Serial2.write(response, sizeof(response));
                Serial2.flush(); // Wait for transmission to complete
                
                // Switch back to receive mode
                digitalWrite(MODBUS_ENABLE_PIN, LOW);
                
                Serial.println("Sent Modbus response");
            } else {
                Serial.printf("Request not for us (addr=%d) or wrong function (func=%d)\n", slaveAddr, funcCode);
            }
        } else {
            Serial.printf("Frame too short: %d bytes\n", rxBufferIndex);
        }
        
        // Reset buffer for next frame
        rxBufferIndex = 0;
    }
}

#else

// Original full application code
#include "adxl355.h"
#include "data_buffer.h"
#include "analytics.h"
#include "modbus_interface.h"
#include "task_manager.h"

// Global objects
ADXL355 sensor;
DataBuffer dataBuffer;
Analytics analytics;
ModbusInterface modbusInterface;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting ADXL355 with FreeRTOS...");
  
  // Initialize sensor
  if (!sensor.begin()) {
    Serial.println("Failed to initialize ADXL355!");
    while(1); // Halt on failure
  }
  
  // Initialize data buffer
  if (!dataBuffer.begin()) {
    Serial.println("Failed to initialize data buffer!");
    while(1); // Halt on failure
  }
  
  // Initialize analytics
  if (!analytics.begin()) {
    Serial.println("Failed to initialize analytics!");
    while(1); // Halt on failure
  }
  
  #if ENABLE_MODBUS_INTERFACE
  // Initialize Modbus interface
  if (!modbusInterface.begin()) {
    Serial.println("Failed to initialize Modbus interface!");
    while(1); // Halt on failure
  }
  #endif
  
  // Start FreeRTOS tasks
  if (!startTasks()) {
    Serial.println("Failed to start tasks!");
    while(1); // Halt on failure
  }
  
  Serial.println("System ready - FreeRTOS tasks running...");
}

void loop() {
  // Main loop is now lightweight - all work is done by FreeRTOS tasks
  
  // Print basic status every 30 seconds (reduced from 10 seconds)
  static unsigned long last_stats_time = 0;
  if (millis() - last_stats_time > 30000) {
    Serial.println("System Status: Running");
    Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
    
    // Print analytics summary only
    if (analytics.isInitialized()) {
      AnalyticsData data = analytics.getAnalyticsData();
      if (data.data_valid) {
        Serial.printf("Analytics - Window: %lu, Valid data available\n", data.window_count);
      }
    }
    
    last_stats_time = millis();
  }
  
  // Small delay to prevent watchdog issues
  delay(500);  // Increased from 100ms to reduce loop frequency
}

#endif