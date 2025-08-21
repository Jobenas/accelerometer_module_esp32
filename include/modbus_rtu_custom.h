#ifndef MODBUS_RTU_CUSTOM_H
#define MODBUS_RTU_CUSTOM_H

#include <Arduino.h>
#include "analytics.h"

// Modbus RTU configuration
#define MODBUS_SLAVE_ID         2     // Modbus slave address
#define MODBUS_BAUDRATE         9600  // Baud rate  
#define MODBUS_TX_PIN           17    // TX pin for Serial2
#define MODBUS_RX_PIN           16    // RX pin for Serial2
#define MODBUS_DE_RE_PIN        4     // Driver Enable / Receiver Enable pin

// Modbus RTU Protocol Constants
#define MODBUS_MAX_FRAME_SIZE   256   // Maximum frame size
#define MODBUS_MIN_FRAME_SIZE   4     // Minimum frame size (slave_id + function + crc)
#define MODBUS_CRC_SIZE         2     // CRC16 size in bytes
#define MODBUS_TIMEOUT_MS       1000  // Response timeout
#define MODBUS_T15_US           750   // 1.5 character time at 9600 baud (~750us)
#define MODBUS_T35_US           1750  // 3.5 character time at 9600 baud (~1750us)

// Modbus Function Codes
#define MODBUS_FC_READ_COILS             0x01
#define MODBUS_FC_READ_DISCRETE_INPUTS   0x02
#define MODBUS_FC_READ_HOLDING_REGISTERS 0x03
#define MODBUS_FC_READ_INPUT_REGISTERS   0x04
#define MODBUS_FC_WRITE_SINGLE_COIL      0x05
#define MODBUS_FC_WRITE_SINGLE_REGISTER  0x06
#define MODBUS_FC_WRITE_MULTIPLE_COILS   0x0F
#define MODBUS_FC_WRITE_MULTIPLE_REGISTERS 0x10

// Modbus Exception Codes
#define MODBUS_EX_ILLEGAL_FUNCTION       0x01
#define MODBUS_EX_ILLEGAL_DATA_ADDRESS   0x02
#define MODBUS_EX_ILLEGAL_DATA_VALUE     0x03
#define MODBUS_EX_SLAVE_DEVICE_FAILURE   0x04

// Register Map - Holding Registers (Read/Write) starting at address 0
#define REG_DEVICE_ID           0     // Device identification
#define REG_FIRMWARE_VERSION    1     // Firmware version
#define REG_SAMPLE_RATE         2     // Sample rate in Hz
#define REG_WINDOW_COUNT_LOW    3     // Window count (lower 16 bits)
#define REG_WINDOW_COUNT_HIGH   4     // Window count (upper 16 bits)

// Register Map - Input Registers (Read-Only) starting at address 0
// Current window statistics
#define REG_CURRENT_AVG_X       0     // Current average X (scaled by 1000)
#define REG_CURRENT_AVG_Y       1     // Current average Y (scaled by 1000)
#define REG_CURRENT_AVG_Z       2     // Current average Z (scaled by 1000)
#define REG_CURRENT_MAX_X       3     // Current maximum X (scaled by 1000)
#define REG_CURRENT_MAX_Y       4     // Current maximum Y (scaled by 1000)
#define REG_CURRENT_MAX_Z       5     // Current maximum Z (scaled by 1000)
#define REG_CURRENT_MIN_X       6     // Current minimum X (scaled by 1000)
#define REG_CURRENT_MIN_Y       7     // Current minimum Y (scaled by 1000)
#define REG_CURRENT_MIN_Z       8     // Current minimum Z (scaled by 1000)
#define REG_CURRENT_STD_X       9     // Current standard deviation X (scaled by 1000)
#define REG_CURRENT_STD_Y       10    // Current standard deviation Y (scaled by 1000)
#define REG_CURRENT_STD_Z       11    // Current standard deviation Z (scaled by 1000)
#define REG_CURRENT_RMS_X       12    // Current RMS X (scaled by 1000)
#define REG_CURRENT_RMS_Y       13    // Current RMS Y (scaled by 1000)
#define REG_CURRENT_RMS_Z       14    // Current RMS Z (scaled by 1000)

// Running statistics
#define REG_RUNNING_AVG_X       15    // Running average X (scaled by 1000)
#define REG_RUNNING_AVG_Y       16    // Running average Y (scaled by 1000)
#define REG_RUNNING_AVG_Z       17    // Running average Z (scaled by 1000)
#define REG_RUNNING_STD_X       18    // Running standard deviation X (scaled by 1000)
#define REG_RUNNING_STD_Y       19    // Running standard deviation Y (scaled by 1000)
#define REG_RUNNING_STD_Z       20    // Running standard deviation Z (scaled by 1000)
#define REG_RUNNING_RMS_X       21    // Running RMS X (scaled by 1000)
#define REG_RUNNING_RMS_Y       22    // Running RMS Y (scaled by 1000)
#define REG_RUNNING_RMS_Z       23    // Running RMS Z (scaled by 1000)
#define REG_GLOBAL_MAX_X        24    // Global maximum X since startup (scaled by 1000)
#define REG_GLOBAL_MAX_Y        25    // Global maximum Y since startup (scaled by 1000)
#define REG_GLOBAL_MAX_Z        26    // Global maximum Z since startup (scaled by 1000)
#define REG_GLOBAL_MIN_X        27    // Global minimum X since startup (scaled by 1000)
#define REG_GLOBAL_MIN_Y        28    // Global minimum Y since startup (scaled by 1000)
#define REG_GLOBAL_MIN_Z        29    // Global minimum Z since startup (scaled by 1000)

// System status registers
#define REG_TASK_STATUS         30    // Task status flags
#define REG_SAMPLING_ERRORS     31    // Sampling error count
#define REG_PROCESSING_ERRORS   32    // Processing error count
#define REG_ANALYTICS_ERRORS    33    // Analytics error count
#define REG_MISSED_SAMPLES      34    // Missed sample count
#define REG_LAST_UPDATE_TIME    35    // Time since last analytics update (ms)

// Configuration constants
#define NUM_HOLDING_REGISTERS   5     // Number of holding registers
#define NUM_INPUT_REGISTERS     36    // Number of input registers (updated for STD/RMS)
#define MODBUS_SCALE_FACTOR     1000  // Scale factor for float values
#define FIRMWARE_VERSION        100   // v1.00

// Frame parsing states
enum ModbusState {
  MODBUS_STATE_IDLE,
  MODBUS_STATE_RECEIVING,
  MODBUS_STATE_PROCESSING,
  MODBUS_STATE_RESPONDING
};

// Statistics for debugging
struct ModbusStats {
  unsigned long frames_received = 0;
  unsigned long frames_processed = 0;
  unsigned long valid_requests = 0;
  unsigned long invalid_requests = 0;
  unsigned long crc_errors = 0;
  unsigned long timeout_errors = 0;
  unsigned long exception_responses = 0;
  unsigned long successful_responses = 0;
  unsigned long last_request_time = 0;
  unsigned long last_response_time = 0;
};

class ModbusRTUCustom {
private:
  // Hardware configuration
  HardwareSerial* serial_port;
  uint8_t slave_id;
  uint8_t de_re_pin;
  
  // State management
  ModbusState current_state;
  bool initialized;
  
  // Frame buffer
  uint8_t rx_buffer[MODBUS_MAX_FRAME_SIZE];
  uint8_t tx_buffer[MODBUS_MAX_FRAME_SIZE];
  uint16_t rx_buffer_index;
  uint16_t tx_buffer_length;
  
  // Timing
  unsigned long last_byte_time;
  unsigned long frame_timeout;
  
  // Register storage
  uint16_t holding_registers[NUM_HOLDING_REGISTERS];
  uint16_t input_registers[NUM_INPUT_REGISTERS];
  
  // Statistics
  ModbusStats stats;
  unsigned long last_update_time;
  
  // Private methods
  void setTransmitMode();
  void setReceiveMode();
  
  // Frame processing
  bool isFrameComplete();
  bool validateFrame(uint8_t* frame, uint16_t length);
  void processFrame();
  void sendResponse();
  void sendExceptionResponse(uint8_t function_code, uint8_t exception_code);
  
  // Function handlers
  void handleReadHoldingRegisters(uint8_t* frame, uint16_t length);
  void handleReadInputRegisters(uint8_t* frame, uint16_t length);
  void handleWriteSingleRegister(uint8_t* frame, uint16_t length);
  void handleWriteMultipleRegisters(uint8_t* frame, uint16_t length);
  
  // CRC calculation (moved calculateCRC16 to public section)
  bool checkCRC(uint8_t* frame, uint16_t length);
  void appendCRC(uint8_t* frame, uint16_t length);
  
  // Register management
  void updateRegistersFromAnalytics();
  int16_t floatToScaledInt(float value);
  uint16_t getTaskStatusFlags();
  
  // Utility functions
  uint16_t bytesToUint16(uint8_t high_byte, uint8_t low_byte);
  void uint16ToBytes(uint16_t value, uint8_t* high_byte, uint8_t* low_byte);
  
public:
  ModbusRTUCustom();
  ~ModbusRTUCustom();
  
  // Initialization and control
  bool begin(uint8_t slave_id = MODBUS_SLAVE_ID, 
             uint32_t baudrate = MODBUS_BAUDRATE,
             uint8_t rx_pin = MODBUS_RX_PIN, 
             uint8_t tx_pin = MODBUS_TX_PIN,
             uint8_t de_re_pin = MODBUS_DE_RE_PIN);
  void update();
  void stop();
  
  // Status and debugging
  bool isInitialized() const { return initialized; }
  ModbusState getState() const { return current_state; }
  const ModbusStats& getStats() const { return stats; }
  void printStats();
  void printRegisterMap();
  void resetStats();
  
  // Register access (for testing/debugging)
  bool setHoldingRegister(uint16_t address, uint16_t value);
  bool setInputRegister(uint16_t address, uint16_t value);
  uint16_t getHoldingRegister(uint16_t address);
  uint16_t getInputRegister(uint16_t address);
  
  // CRC testing (for debugging) - moved from private section
  uint16_t calculateCRC16(uint8_t* data, uint16_t length);
};

// Global instance
extern ModbusRTUCustom modbusRTU;

#endif // MODBUS_RTU_CUSTOM_H
