#include "modbus_rtu_custom.h"
#include "config.h"
#include "task_manager.h"

// Global instance
ModbusRTUCustom modbusRTU;

// CRC16 lookup table for faster calculation
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

ModbusRTUCustom::ModbusRTUCustom() {
  serial_port = &Serial2;
  slave_id = MODBUS_SLAVE_ID;
  de_re_pin = MODBUS_DE_RE_PIN;
  current_state = MODBUS_STATE_IDLE;
  initialized = false;
  rx_buffer_index = 0;
  tx_buffer_length = 0;
  last_byte_time = 0;
  frame_timeout = MODBUS_T35_US;
  last_update_time = 0;
  
  // Initialize registers with default values
  memset(holding_registers, 0, sizeof(holding_registers));
  memset(input_registers, 0, sizeof(input_registers));
  
  // Set default holding register values
  holding_registers[REG_DEVICE_ID] = 0x1234;  // Device ID
  holding_registers[REG_FIRMWARE_VERSION] = FIRMWARE_VERSION;
  holding_registers[REG_SAMPLE_RATE] = 1000;  // 1kHz default
  
  // Reset statistics
  memset(&stats, 0, sizeof(stats));
}

ModbusRTUCustom::~ModbusRTUCustom() {
  stop();
}

bool ModbusRTUCustom::begin(uint8_t slave_id, uint32_t baudrate, uint8_t rx_pin, uint8_t tx_pin, uint8_t de_re_pin) {
  this->slave_id = slave_id;
  this->de_re_pin = de_re_pin;
  
  // Configure DE/RE pin
  pinMode(de_re_pin, OUTPUT);
  setReceiveMode();
  
  // Initialize serial port
  serial_port->begin(baudrate, SERIAL_8N1, rx_pin, tx_pin);
  
  // Clear buffers
  rx_buffer_index = 0;
  current_state = MODBUS_STATE_IDLE;
  
  initialized = true;
  
  #if ENABLE_DEBUG_OUTPUT
  Serial.printf("[Modbus] Initialized - Slave ID: %d, Baudrate: %lu\n", slave_id, baudrate);
  Serial.printf("[Modbus] Pins - RX: %d, TX: %d, DE/RE: %d\n", rx_pin, tx_pin, de_re_pin);
  #endif
  
  return true;
}

void ModbusRTUCustom::stop() {
  if (initialized) {
    serial_port->end();
    initialized = false;
    current_state = MODBUS_STATE_IDLE;
    
    #if ENABLE_DEBUG_OUTPUT
    Serial.println("[Modbus] Stopped");
    #endif
  }
}

void ModbusRTUCustom::update() {
  if (!initialized) return;
  
  unsigned long current_time = micros();
  
  // Update registers from analytics data
  if (millis() - last_update_time > 100) {  // Update every 100ms
    updateRegistersFromAnalytics();
    last_update_time = millis();
  }
  
  // Handle incoming bytes
  if (serial_port->available()) {
    #if ENABLE_DEBUG_OUTPUT
    Serial.printf("[Modbus] Data available on Serial2: %d bytes\n", serial_port->available());
    #endif
    
    while (serial_port->available()) {
      uint8_t byte_received = serial_port->read();
      
      // Check for buffer overflow
      if (rx_buffer_index >= MODBUS_MAX_FRAME_SIZE) {
        #if ENABLE_DEBUG_OUTPUT
        Serial.println("[Modbus] Buffer overflow, resetting");
        #endif
        rx_buffer_index = 0;
        current_state = MODBUS_STATE_IDLE;
        stats.timeout_errors++;
        continue;
      }
      
      rx_buffer[rx_buffer_index++] = byte_received;
      last_byte_time = current_time;
      current_state = MODBUS_STATE_RECEIVING;
      
      #if ENABLE_DEBUG_OUTPUT
      Serial.printf("[Modbus] RX[%d]: 0x%02X\n", rx_buffer_index-1, byte_received);
      #endif
    }
  }
  
  // Check for frame completion (T3.5 silence)
  if (current_state == MODBUS_STATE_RECEIVING) {
    if (current_time - last_byte_time > frame_timeout) {
      if (isFrameComplete()) {
        current_state = MODBUS_STATE_PROCESSING;
        stats.frames_received++;
        processFrame();
      } else {
        // Invalid frame, reset
        #if ENABLE_DEBUG_OUTPUT
        Serial.printf("[Modbus] Invalid frame length: %d\n", rx_buffer_index);
        #endif
        rx_buffer_index = 0;
        current_state = MODBUS_STATE_IDLE;
        stats.invalid_requests++;
      }
    }
  }
}

void ModbusRTUCustom::setTransmitMode() {
  digitalWrite(de_re_pin, HIGH);
  delayMicroseconds(10);  // Small delay for pin settling
}

void ModbusRTUCustom::setReceiveMode() {
  digitalWrite(de_re_pin, LOW);
  delayMicroseconds(10);  // Small delay for pin settling
}

bool ModbusRTUCustom::isFrameComplete() {
  return (rx_buffer_index >= MODBUS_MIN_FRAME_SIZE && rx_buffer_index <= MODBUS_MAX_FRAME_SIZE);
}

bool ModbusRTUCustom::validateFrame(uint8_t* frame, uint16_t length) {
  if (length < MODBUS_MIN_FRAME_SIZE) return false;
  if (frame[0] != slave_id) return false;  // Not for this slave
  return checkCRC(frame, length);
}

void ModbusRTUCustom::processFrame() {
  #if ENABLE_DEBUG_OUTPUT
  Serial.println("[MODBUS DEBUG] === PROCESSING FRAME ===");
  Serial.printf("[MODBUS DEBUG] Frame length: %d bytes\n", rx_buffer_index);
  Serial.print("[MODBUS DEBUG] Frame content: ");
  for (int i = 0; i < rx_buffer_index; i++) {
    Serial.printf("0x%02X ", rx_buffer[i]);
  }
  Serial.println();
  
  // Check slave address first
  if (rx_buffer_index > 0) {
    Serial.printf("[MODBUS DEBUG] Slave ID: received=0x%02X, expected=0x%02X\n", 
                 rx_buffer[0], slave_id);
  }
  #endif
  
  if (!validateFrame(rx_buffer, rx_buffer_index)) {
    #if ENABLE_DEBUG_OUTPUT
    Serial.println("[Modbus] Frame validation failed");
    Serial.println("[MODBUS DEBUG] === FRAME PROCESSING FAILED ===\n");
    #endif
    if (!checkCRC(rx_buffer, rx_buffer_index)) {
      stats.crc_errors++;
    }
    stats.invalid_requests++;
    rx_buffer_index = 0;
    current_state = MODBUS_STATE_IDLE;
    return;
  }
  
  stats.frames_processed++;
  stats.last_request_time = millis();
  
  uint8_t function_code = rx_buffer[1];
  
  #if ENABLE_DEBUG_OUTPUT
  Serial.printf("[Modbus] Processing function code: 0x%02X\n", function_code);
  #endif
  
  switch (function_code) {
    case MODBUS_FC_READ_HOLDING_REGISTERS:
      handleReadHoldingRegisters(rx_buffer, rx_buffer_index);
      break;
      
    case MODBUS_FC_READ_INPUT_REGISTERS:
      handleReadInputRegisters(rx_buffer, rx_buffer_index);
      break;
      
    case MODBUS_FC_WRITE_SINGLE_REGISTER:
      handleWriteSingleRegister(rx_buffer, rx_buffer_index);
      break;
      
    case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
      handleWriteMultipleRegisters(rx_buffer, rx_buffer_index);
      break;
      
    default:
      #if ENABLE_DEBUG_OUTPUT
      Serial.printf("[Modbus] Unsupported function code: 0x%02X\n", function_code);
      #endif
      sendExceptionResponse(function_code, MODBUS_EX_ILLEGAL_FUNCTION);
      break;
  }
  
  // Reset for next frame
  rx_buffer_index = 0;
  current_state = MODBUS_STATE_IDLE;
  
  #if ENABLE_DEBUG_OUTPUT
  Serial.println("[MODBUS DEBUG] === FRAME PROCESSING COMPLETE ===\n");
  #endif
}

void ModbusRTUCustom::handleReadHoldingRegisters(uint8_t* frame, uint16_t length) {
  if (length != 8) {  // Slave ID + Function + Start Address + Quantity + CRC = 8 bytes
    sendExceptionResponse(frame[1], MODBUS_EX_ILLEGAL_DATA_VALUE);
    return;
  }
  
  uint16_t start_address = bytesToUint16(frame[2], frame[3]);
  uint16_t quantity = bytesToUint16(frame[4], frame[5]);
  
  if (quantity == 0 || quantity > 125 || start_address + quantity > NUM_HOLDING_REGISTERS) {
    sendExceptionResponse(frame[1], MODBUS_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }
  
  // Build response
  tx_buffer[0] = slave_id;
  tx_buffer[1] = frame[1];  // Function code
  tx_buffer[2] = quantity * 2;  // Byte count
  
  for (uint16_t i = 0; i < quantity; i++) {
    uint16_t reg_value = holding_registers[start_address + i];
    uint16ToBytes(reg_value, &tx_buffer[3 + i*2], &tx_buffer[4 + i*2]);
  }
  
  tx_buffer_length = 3 + quantity * 2;
  appendCRC(tx_buffer, tx_buffer_length);
  tx_buffer_length += 2;
  
  sendResponse();
  stats.valid_requests++;
}

void ModbusRTUCustom::handleReadInputRegisters(uint8_t* frame, uint16_t length) {
  if (length != 8) {
    sendExceptionResponse(frame[1], MODBUS_EX_ILLEGAL_DATA_VALUE);
    return;
  }
  
  uint16_t start_address = bytesToUint16(frame[2], frame[3]);
  uint16_t quantity = bytesToUint16(frame[4], frame[5]);
  
  if (quantity == 0 || quantity > 125 || start_address + quantity > NUM_INPUT_REGISTERS) {
    sendExceptionResponse(frame[1], MODBUS_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }
  
  // Build response
  tx_buffer[0] = slave_id;
  tx_buffer[1] = frame[1];  // Function code
  tx_buffer[2] = quantity * 2;  // Byte count
  
  for (uint16_t i = 0; i < quantity; i++) {
    uint16_t reg_value = input_registers[start_address + i];
    uint16ToBytes(reg_value, &tx_buffer[3 + i*2], &tx_buffer[4 + i*2]);
  }
  
  tx_buffer_length = 3 + quantity * 2;
  appendCRC(tx_buffer, tx_buffer_length);
  tx_buffer_length += 2;
  
  sendResponse();
  stats.valid_requests++;
}

void ModbusRTUCustom::handleWriteSingleRegister(uint8_t* frame, uint16_t length) {
  if (length != 8) {
    sendExceptionResponse(frame[1], MODBUS_EX_ILLEGAL_DATA_VALUE);
    return;
  }
  
  uint16_t address = bytesToUint16(frame[2], frame[3]);
  uint16_t value = bytesToUint16(frame[4], frame[5]);
  
  if (address >= NUM_HOLDING_REGISTERS) {
    sendExceptionResponse(frame[1], MODBUS_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }
  
  holding_registers[address] = value;
  
  // Echo the request as response
  memcpy(tx_buffer, frame, length);
  tx_buffer_length = length;
  
  sendResponse();
  stats.valid_requests++;
}

void ModbusRTUCustom::handleWriteMultipleRegisters(uint8_t* frame, uint16_t length) {
  if (length < 9) {  // Minimum: Slave ID + Function + Start + Quantity + Byte Count + 1 Register + CRC
    sendExceptionResponse(frame[1], MODBUS_EX_ILLEGAL_DATA_VALUE);
    return;
  }
  
  uint16_t start_address = bytesToUint16(frame[2], frame[3]);
  uint16_t quantity = bytesToUint16(frame[4], frame[5]);
  uint8_t byte_count = frame[6];
  
  if (quantity == 0 || quantity > 123 || byte_count != quantity * 2 || 
      start_address + quantity > NUM_HOLDING_REGISTERS ||
      length != 9 + byte_count) {
    sendExceptionResponse(frame[1], MODBUS_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }
  
  // Write registers
  for (uint16_t i = 0; i < quantity; i++) {
    uint16_t value = bytesToUint16(frame[7 + i*2], frame[8 + i*2]);
    holding_registers[start_address + i] = value;
  }
  
  // Build response
  tx_buffer[0] = slave_id;
  tx_buffer[1] = frame[1];  // Function code
  uint16ToBytes(start_address, &tx_buffer[2], &tx_buffer[3]);
  uint16ToBytes(quantity, &tx_buffer[4], &tx_buffer[5]);
  
  tx_buffer_length = 6;
  appendCRC(tx_buffer, tx_buffer_length);
  tx_buffer_length += 2;
  
  sendResponse();
  stats.valid_requests++;
}

void ModbusRTUCustom::sendResponse() {
  setTransmitMode();
  
  serial_port->write(tx_buffer, tx_buffer_length);
  serial_port->flush();  // Wait for transmission to complete
  
  setReceiveMode();
  
  stats.successful_responses++;
  stats.last_response_time = millis();
  
  #if ENABLE_DEBUG_OUTPUT
  Serial.printf("[Modbus] Response sent: %d bytes\n", tx_buffer_length);
  #if ENABLE_VERBOSE_DEBUG
  Serial.print("[Modbus] TX: ");
  for (uint16_t i = 0; i < tx_buffer_length; i++) {
    Serial.printf("0x%02X ", tx_buffer[i]);
  }
  Serial.println();
  #endif
  #endif
}

void ModbusRTUCustom::sendExceptionResponse(uint8_t function_code, uint8_t exception_code) {
  tx_buffer[0] = slave_id;
  tx_buffer[1] = function_code | 0x80;  // Set exception bit
  tx_buffer[2] = exception_code;
  
  tx_buffer_length = 3;
  appendCRC(tx_buffer, tx_buffer_length);
  tx_buffer_length += 2;
  
  sendResponse();
  stats.exception_responses++;
  
  #if ENABLE_DEBUG_OUTPUT
  Serial.printf("[Modbus] Exception response - Function: 0x%02X, Exception: 0x%02X\n", 
                function_code, exception_code);
  #endif
}

uint16_t ModbusRTUCustom::calculateCRC16(uint8_t* data, uint16_t length) {
  uint16_t crc = 0xFFFF;
  
  for (uint16_t i = 0; i < length; i++) {
    uint8_t index = (crc ^ data[i]) & 0xFF;
    crc = (crc >> 8) ^ crc16_table[index];
  }
  
  return crc;
}

bool ModbusRTUCustom::checkCRC(uint8_t* frame, uint16_t length) {
  if (length < 3) {
    #if ENABLE_DEBUG_OUTPUT
    Serial.println("[CRC DEBUG] Frame too short for CRC check");
    #endif
    return false;
  }
  
  // Calculate CRC for data (excluding the CRC bytes)
  uint16_t calculated_crc = calculateCRC16(frame, length - 2);
  
  // Extract received CRC (last 2 bytes) - Modbus CRC is little-endian
  uint16_t received_crc = bytesToUint16(frame[length-1], frame[length-2]);
  
  #if ENABLE_DEBUG_OUTPUT
  Serial.printf("[CRC DEBUG] Frame length: %d\n", length);
  Serial.printf("[CRC DEBUG] Data bytes: ");
  for (int i = 0; i < length - 2; i++) {
    Serial.printf("0x%02X ", frame[i]);
  }
  Serial.println();
  Serial.printf("[CRC DEBUG] Received CRC: 0x%04X (bytes: 0x%02X 0x%02X)\n", 
               received_crc, frame[length-2], frame[length-1]);
  Serial.printf("[CRC DEBUG] Calculated CRC: 0x%04X\n", calculated_crc);
  Serial.printf("[CRC DEBUG] CRC Match: %s\n", (calculated_crc == received_crc) ? "YES" : "NO");
  #endif
  
  return (calculated_crc == received_crc);
}

void ModbusRTUCustom::appendCRC(uint8_t* frame, uint16_t length) {
  uint16_t crc = calculateCRC16(frame, length);
  frame[length] = crc & 0xFF;       // Low byte first
  frame[length + 1] = crc >> 8;     // High byte second
}

void ModbusRTUCustom::updateRegistersFromAnalytics() {
  // Only update if analytics is available and initialized
  extern Analytics analytics;
  if (!analytics.isInitialized()) {
    #if ENABLE_DEBUG_OUTPUT
    Serial.println("[Modbus] Analytics not initialized - using test values");
    #endif
    // Provide test values when analytics is not available
    input_registers[REG_CURRENT_AVG_X] = 100;   // 0.1g
    input_registers[REG_CURRENT_AVG_Y] = 200;   // 0.2g  
    input_registers[REG_CURRENT_AVG_Z] = 1000;  // 1.0g (gravity)
    input_registers[REG_CURRENT_MAX_X] = 150;
    input_registers[REG_CURRENT_MAX_Y] = 250;
    input_registers[REG_CURRENT_MAX_Z] = 1100;
    input_registers[REG_CURRENT_MIN_X] = 50;
    input_registers[REG_CURRENT_MIN_Y] = 150;
    input_registers[REG_CURRENT_MIN_Z] = 900;
    return;
  }
  
  AnalyticsData data = analytics.getAnalyticsData();
  if (!data.data_valid) {
    #if ENABLE_DEBUG_OUTPUT
    Serial.println("[Modbus] Analytics data not valid - using test values");
    #endif
    // Provide test values when analytics data is invalid
    input_registers[REG_CURRENT_AVG_X] = 300;   // 0.3g
    input_registers[REG_CURRENT_AVG_Y] = 400;   // 0.4g  
    input_registers[REG_CURRENT_AVG_Z] = 1000;  // 1.0g (gravity)
    input_registers[REG_CURRENT_MAX_X] = 350;
    input_registers[REG_CURRENT_MAX_Y] = 450;
    input_registers[REG_CURRENT_MAX_Z] = 1100;
    input_registers[REG_CURRENT_MIN_X] = 250;
    input_registers[REG_CURRENT_MIN_Y] = 350;
    input_registers[REG_CURRENT_MIN_Z] = 900;
    return;
  }
  
  #if ENABLE_DEBUG_OUTPUT
  static unsigned long last_modbus_debug = 0;
  if (millis() - last_modbus_debug > 2000) {  // Debug every 2 seconds
    Serial.printf("[Modbus-DEBUG] Analytics data (g-values) - X: %.6f, Y: %.6f, Z: %.6f\n", 
                  data.current_avg_x, data.current_avg_y, data.current_avg_z);
    Serial.printf("[Modbus-DEBUG] Max values (g-values) - X: %.6f, Y: %.6f, Z: %.6f\n", 
                  data.current_max_x, data.current_max_y, data.current_max_z);
    Serial.printf("[Modbus-DEBUG] STD values (g-values) - X: %.6f, Y: %.6f, Z: %.6f\n", 
                  data.current_std_x, data.current_std_y, data.current_std_z);
    Serial.printf("[Modbus-DEBUG] RMS values (g-values) - X: %.6f, Y: %.6f, Z: %.6f\n", 
                  data.current_rms_x, data.current_rms_y, data.current_rms_z);
    
    // Show what happens when we convert to scaled integers
    int32_t scaled_x = (int32_t)(data.current_avg_x * MODBUS_SCALE_FACTOR);
    int32_t scaled_y = (int32_t)(data.current_avg_y * MODBUS_SCALE_FACTOR);
    int32_t scaled_z = (int32_t)(data.current_avg_z * MODBUS_SCALE_FACTOR);
    Serial.printf("[Modbus-DEBUG] Scaled for Modbus: X=%ld, Y=%ld, Z=%ld (should be in ±32767 range)\n", 
                  scaled_x, scaled_y, scaled_z);
    
    last_modbus_debug = millis();
  }
  #endif
  
  // Update current window statistics
  input_registers[REG_CURRENT_AVG_X] = floatToScaledInt(data.current_avg_x);
  input_registers[REG_CURRENT_AVG_Y] = floatToScaledInt(data.current_avg_y);
  input_registers[REG_CURRENT_AVG_Z] = floatToScaledInt(data.current_avg_z);
  input_registers[REG_CURRENT_MAX_X] = floatToScaledInt(data.current_max_x);
  input_registers[REG_CURRENT_MAX_Y] = floatToScaledInt(data.current_max_y);
  input_registers[REG_CURRENT_MAX_Z] = floatToScaledInt(data.current_max_z);
  input_registers[REG_CURRENT_MIN_X] = floatToScaledInt(data.current_min_x);
  input_registers[REG_CURRENT_MIN_Y] = floatToScaledInt(data.current_min_y);
  input_registers[REG_CURRENT_MIN_Z] = floatToScaledInt(data.current_min_z);
  input_registers[REG_CURRENT_STD_X] = floatToScaledInt(data.current_std_x);
  input_registers[REG_CURRENT_STD_Y] = floatToScaledInt(data.current_std_y);
  input_registers[REG_CURRENT_STD_Z] = floatToScaledInt(data.current_std_z);
  input_registers[REG_CURRENT_RMS_X] = floatToScaledInt(data.current_rms_x);
  input_registers[REG_CURRENT_RMS_Y] = floatToScaledInt(data.current_rms_y);
  input_registers[REG_CURRENT_RMS_Z] = floatToScaledInt(data.current_rms_z);
  
  // Update running statistics
  input_registers[REG_RUNNING_AVG_X] = floatToScaledInt(data.running_avg_x);
  input_registers[REG_RUNNING_AVG_Y] = floatToScaledInt(data.running_avg_y);
  input_registers[REG_RUNNING_AVG_Z] = floatToScaledInt(data.running_avg_z);
  input_registers[REG_RUNNING_STD_X] = floatToScaledInt(data.running_std_x);
  input_registers[REG_RUNNING_STD_Y] = floatToScaledInt(data.running_std_y);
  input_registers[REG_RUNNING_STD_Z] = floatToScaledInt(data.running_std_z);
  input_registers[REG_RUNNING_RMS_X] = floatToScaledInt(data.running_rms_x);
  input_registers[REG_RUNNING_RMS_Y] = floatToScaledInt(data.running_rms_y);
  input_registers[REG_RUNNING_RMS_Z] = floatToScaledInt(data.running_rms_z);
  input_registers[REG_GLOBAL_MAX_X] = floatToScaledInt(data.global_max_x);
  input_registers[REG_GLOBAL_MAX_Y] = floatToScaledInt(data.global_max_y);
  input_registers[REG_GLOBAL_MAX_Z] = floatToScaledInt(data.global_max_z);
  input_registers[REG_GLOBAL_MIN_X] = floatToScaledInt(data.global_min_x);
  input_registers[REG_GLOBAL_MIN_Y] = floatToScaledInt(data.global_min_y);
  input_registers[REG_GLOBAL_MIN_Z] = floatToScaledInt(data.global_min_z);
  
  // Update system status
  input_registers[REG_TASK_STATUS] = getTaskStatusFlags();
  
  // Update window count (split into two 16-bit registers)
  holding_registers[REG_WINDOW_COUNT_LOW] = data.window_count & 0xFFFF;
  holding_registers[REG_WINDOW_COUNT_HIGH] = (data.window_count >> 16) & 0xFFFF;
  
  // Update error counts and timing
  extern TaskManagerStatus task_status;
  input_registers[REG_SAMPLING_ERRORS] = task_status.sampling_errors & 0xFFFF;
  input_registers[REG_PROCESSING_ERRORS] = task_status.processing_errors & 0xFFFF;
  input_registers[REG_ANALYTICS_ERRORS] = task_status.analytics_errors & 0xFFFF;
  input_registers[REG_MISSED_SAMPLES] = task_status.missed_samples & 0xFFFF;
  input_registers[REG_LAST_UPDATE_TIME] = (millis() - data.last_update_time) & 0xFFFF;
}

int16_t ModbusRTUCustom::floatToScaledInt(float value) {
  // Scale by 1000 and clamp to int16 range
  int32_t scaled = (int32_t)(value * MODBUS_SCALE_FACTOR);
  
  #if ENABLE_DEBUG_OUTPUT
  static unsigned long last_clamp_debug = 0;
  if ((scaled > 32767 || scaled < -32768) && (millis() - last_clamp_debug > 1000)) {
    Serial.printf("[Modbus-CLAMP] Clamping value: %.6f -> %ld (clamped to %d)\n", 
                  value, scaled, (scaled > 32767) ? 32767 : -32768);
    last_clamp_debug = millis();
  }
  #endif
  
  if (scaled > 32767) scaled = 32767;
  if (scaled < -32768) scaled = -32768;
  return (int16_t)scaled;
}

uint16_t ModbusRTUCustom::getTaskStatusFlags() {
  extern TaskManagerStatus task_status;
  uint16_t flags = 0;
  
  if (task_status.sampling_task_running) flags |= 0x0001;
  if (task_status.processing_task_running) flags |= 0x0002;
  if (task_status.analytics_task_running) flags |= 0x0004;
  if (task_status.modbus_task_running) flags |= 0x0008;
  
  return flags;
}

uint16_t ModbusRTUCustom::bytesToUint16(uint8_t high_byte, uint8_t low_byte) {
  return ((uint16_t)high_byte << 8) | low_byte;
}

void ModbusRTUCustom::uint16ToBytes(uint16_t value, uint8_t* high_byte, uint8_t* low_byte) {
  *high_byte = (value >> 8) & 0xFF;
  *low_byte = value & 0xFF;
}

// Public utility functions for testing/debugging
bool ModbusRTUCustom::setHoldingRegister(uint16_t address, uint16_t value) {
  if (address >= NUM_HOLDING_REGISTERS) return false;
  holding_registers[address] = value;
  return true;
}

bool ModbusRTUCustom::setInputRegister(uint16_t address, uint16_t value) {
  if (address >= NUM_INPUT_REGISTERS) return false;
  input_registers[address] = value;
  return true;
}

uint16_t ModbusRTUCustom::getHoldingRegister(uint16_t address) {
  if (address >= NUM_HOLDING_REGISTERS) return 0;
  return holding_registers[address];
}

uint16_t ModbusRTUCustom::getInputRegister(uint16_t address) {
  if (address >= NUM_INPUT_REGISTERS) return 0;
  return input_registers[address];
}

void ModbusRTUCustom::printStats() {
  #if ENABLE_DEBUG_OUTPUT
  Serial.println("\n=== Modbus RTU Statistics ===");
  Serial.printf("State: %d, Initialized: %s\n", current_state, initialized ? "Yes" : "No");
  Serial.printf("Frames Received: %lu\n", stats.frames_received);
  Serial.printf("Frames Processed: %lu\n", stats.frames_processed);
  Serial.printf("Valid Requests: %lu\n", stats.valid_requests);
  Serial.printf("Invalid Requests: %lu\n", stats.invalid_requests);
  Serial.printf("CRC Errors: %lu\n", stats.crc_errors);
  Serial.printf("Timeout Errors: %lu\n", stats.timeout_errors);
  Serial.printf("Exception Responses: %lu\n", stats.exception_responses);
  Serial.printf("Successful Responses: %lu\n", stats.successful_responses);
  Serial.printf("Last Request: %lu ms ago\n", millis() - stats.last_request_time);
  Serial.printf("Last Response: %lu ms ago\n", millis() - stats.last_response_time);
  Serial.println("=============================\n");
  #endif
}

void ModbusRTUCustom::printRegisterMap() {
  #if ENABLE_DEBUG_OUTPUT
  Serial.println("\n=== Modbus Register Map ===");
  Serial.println("Holding Registers (Read/Write):");
  for (uint16_t i = 0; i < NUM_HOLDING_REGISTERS; i++) {
    Serial.printf("  [%2d]: %5d (0x%04X)\n", i, holding_registers[i], holding_registers[i]);
  }
  
  Serial.println("\nInput Registers (Read-Only):");
  for (uint16_t i = 0; i < NUM_INPUT_REGISTERS; i++) {
    Serial.printf("  [%2d]: %5d (0x%04X)\n", i, (int16_t)input_registers[i], input_registers[i]);
  }
  Serial.println("===========================\n");
  #endif
}

void ModbusRTUCustom::resetStats() {
  memset(&stats, 0, sizeof(stats));
}
