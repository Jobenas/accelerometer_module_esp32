#ifndef CONFIG_H
#define CONFIG_H

// Debug configuration - set to false to reduce serial output
#define ENABLE_DEBUG_OUTPUT     true   // General debug output
#define ENABLE_VERBOSE_DEBUG    true   // Detailed debug output - ENABLED FOR DEBUGGING
#define ENABLE_TASK_DEBUG       true   // Task performance debug - ENABLED FOR DEBUGGING
#define ENABLE_ANALYTICS_DEBUG  true   // Analytics debug output
#define ENABLE_MODBUS_INTERFACE true   // Enable/disable Modbus interface
#define ENABLE_MODBUS_DEBUG     true   // Enable Modbus communication debug

// Hardware pin definitions
#define CS_PIN 5
#define MOSI_PIN 23
#define MISO_PIN 19
#define SCLK_PIN 18
#define POWER_EN 15  // GPIO connected to ADXL355 VDD

// ADXL355 Register addresses
#define DEVID_AD      0x00
#define DEVID_MST     0x01
#define PARTID        0x02
#define STATUS        0x04
#define XDATA3        0x08
#define POWER_CTL     0x2D

// Expected device IDs
#define EXPECTED_DEVID_AD   0xAD
#define EXPECTED_PARTID     0xED

// Communication settings
#define SPI_MODE          0
#define SPI_FREQUENCY     1000000  // 1 MHz

#endif // CONFIG_H
