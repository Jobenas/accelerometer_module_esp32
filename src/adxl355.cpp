#include "adxl355.h"

ADXL355::ADXL355() : initialized(false) {
}

bool ADXL355::begin() {
  // Set MOSI low initially
  pinMode(MOSI_PIN, OUTPUT);
  digitalWrite(MOSI_PIN, LOW);

  // Power sequencing
  pinMode(POWER_EN, OUTPUT);
  digitalWrite(POWER_EN, LOW);
  delay(100);
  digitalWrite(POWER_EN, HIGH);
  delay(100);

  // Initialize SPI
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  SPI.begin(SCLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE));

  // Initialize ADXL355
  writeRegister(POWER_CTL, 0x00);  // Reset
  delay(50);
  writeRegister(POWER_CTL, 0x06);  // Enable measurement mode
  delay(50);

  // Verify device ID
  if (checkDeviceID()) {
    initialized = true;
    Serial.println("ADXL355 initialized successfully");
    Serial.printf("Scale factor: %.1f (corrected)\n", getScaleFactor());
    printDeviceInfo();
    return true;
  } else {
    Serial.println("ADXL355 initialization failed - wrong device ID");
    return false;
  }
}

void ADXL355::end() {
  if (initialized) {
    writeRegister(POWER_CTL, 0x01);  // Standby mode
    SPI.end();
    initialized = false;
  }
}

void ADXL355::writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer((reg << 1) | 0x00);  // Write command
  SPI.transfer(value);
  digitalWrite(CS_PIN, HIGH);
}

uint8_t ADXL355::readRegister(uint8_t reg) {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer((reg << 1) | 0x01);  // Read command
  uint8_t value = SPI.transfer(0x00);
  digitalWrite(CS_PIN, HIGH);
  return value;
}

void ADXL355::readXYZ(int32_t &x, int32_t &y, int32_t &z) {
  uint8_t buffer[9];
  
  // Read all 9 bytes (3 axes * 3 bytes each) in one transaction
  digitalWrite(CS_PIN, LOW);
  SPI.transfer((XDATA3 << 1) | 0x01);  // Read command for XDATA3
  for (int i = 0; i < 9; i++) {
    buffer[i] = SPI.transfer(0x00);
  }
  digitalWrite(CS_PIN, HIGH);

  #if ENABLE_VERBOSE_DEBUG
  static unsigned long last_spi_debug = 0;
  if (millis() - last_spi_debug > 10000) {  // SPI debug every 10 seconds
    Serial.print("[ADXL355-SPI] Raw bytes: ");
    for (int i = 0; i < 9; i++) {
      Serial.printf("0x%02X ", buffer[i]);
    }
    Serial.println();
    last_spi_debug = millis();
  }
  #endif

  // Convert to 20-bit signed values
  x = ((int32_t)buffer[0] << 12) | ((int32_t)buffer[1] << 4) | (buffer[2] >> 4);
  y = ((int32_t)buffer[3] << 12) | ((int32_t)buffer[4] << 4) | (buffer[5] >> 4);
  z = ((int32_t)buffer[6] << 12) | ((int32_t)buffer[7] << 4) | (buffer[8] >> 4);

  #if ENABLE_VERBOSE_DEBUG
  if (millis() - last_spi_debug > 9900 && millis() - last_spi_debug < 10100) {  // Show conversion details
    Serial.printf("[ADXL355-SPI] Before sign extend: X=%ld Y=%ld Z=%ld\n", x, y, z);
  }
  #endif

  // Sign extend from 20-bit to 32-bit
  if (x & 0x80000) x |= 0xFFF00000;
  if (y & 0x80000) y |= 0xFFF00000;
  if (z & 0x80000) z |= 0xFFF00000;

  #if ENABLE_VERBOSE_DEBUG
  if (millis() - last_spi_debug > 9900 && millis() - last_spi_debug < 10100) {  // Show final values
    Serial.printf("[ADXL355-SPI] After sign extend: X=%ld Y=%ld Z=%ld\n", x, y, z);
  }
  #endif
}

void ADXL355::readAcceleration(float &x_g, float &y_g, float &z_g) {
  #if ENABLE_VERBOSE_DEBUG
  static unsigned long call_count = 0;
  call_count++;
  static unsigned long last_call_debug = 0;
  if (millis() - last_call_debug > 3000) {  // Show call frequency every 3 seconds
    Serial.printf("[ADXL355-CALLS] readAcceleration called %lu times in last 3 sec\n", call_count);
    call_count = 0;
    last_call_debug = millis();
  }
  #endif
  
  int32_t x_raw, y_raw, z_raw;
  readXYZ(x_raw, y_raw, z_raw);
  
  // ADXL355 ±2g range: Corrected scale factor based on observed values
  // Values were ~250,000x too large, so use much larger scale factor
  static const float scale_factor = 256000.0f;  // Corrected scale factor (was 65536000.0f)
  
  x_g = (float)x_raw / scale_factor;
  y_g = (float)y_raw / scale_factor;
  z_g = (float)z_raw / scale_factor;
  
  #if ENABLE_DEBUG_OUTPUT
  static unsigned long last_debug = 0;
  if (millis() - last_debug > 5000) {  // Debug every 5 seconds
    Serial.printf("[ADXL355] Raw: %ld, %ld, %ld -> G: %.6f, %.6f, %.6f\n", 
                  x_raw, y_raw, z_raw, x_g, y_g, z_g);
    Serial.printf("[ADXL355] Scale factor used: %.1f\n", scale_factor);
    last_debug = millis();
  }
  #endif
  
  #if ENABLE_VERBOSE_DEBUG
  static unsigned long last_verbose = 0;
  if (millis() - last_verbose > 1000) {  // Verbose debug every 1 second
    Serial.printf("[ADXL355-VERBOSE] Raw: X=%ld Y=%ld Z=%ld\n", x_raw, y_raw, z_raw);
    Serial.printf("[ADXL355-VERBOSE] Converted: X=%.6f Y=%.6f Z=%.6f\n", x_g, y_g, z_g);
    last_verbose = millis();
  }
  #endif
}

bool ADXL355::checkDeviceID() {
  uint8_t devid_ad = readRegister(DEVID_AD);
  uint8_t partid = readRegister(PARTID);
  
  return (devid_ad == EXPECTED_DEVID_AD && partid == EXPECTED_PARTID);
}

void ADXL355::printDeviceInfo() {
  uint8_t devid_ad = readRegister(DEVID_AD);
  uint8_t devid_mst = readRegister(DEVID_MST);
  uint8_t partid = readRegister(PARTID);
  
  Serial.print("DEVID_AD: 0x"); Serial.println(devid_ad, HEX);
  Serial.print("DEVID_MST: 0x"); Serial.println(devid_mst, HEX);
  Serial.print("PARTID: 0x"); Serial.println(partid, HEX);
}

float ADXL355::getScaleFactor() const {
  // Return the same scale factor used in readAcceleration
  return 256000.0f;  // Corrected scale factor (was 65536000.0f)
}
