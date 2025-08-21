# Accelerometer Abstraction Implementation

This implementation provides a unified interface for both ADXL355 and MPU6050 accelerometers, allowing you to switch between them using compile-time configuration.

## Quick Start

### 1. Choose Your Accelerometer

Edit `include/accelerometer_config.h`:

**For ADXL355:**
```cpp
#define USE_ADXL355
// #define USE_MPU6050
```

**For MPU6050:**
```cpp
// #define USE_ADXL355
#define USE_MPU6050
```

### 2. Build and Flash

```bash
pio run -t upload
```

### 3. Monitor Output

```bash
pio device monitor
```

## File Structure

```
include/
├── accelerometer_config.h      # Sensor selection and common interface
├── accelerometer_interface.h   # High-level wrapper class
├── adxl355.h                  # Original ADXL355 specific code
└── ... (other headers)

src/
├── accelerometer_adxl355.cpp   # ADXL355 implementation
├── accelerometer_mpu6050.cpp   # MPU6050 implementation
├── accelerometer_interface.cpp # Wrapper implementation
├── main.cpp                   # Updated to use abstraction
├── task_manager.cpp           # Updated to use abstraction
└── ... (other source files)

test/
└── test_accelerometer_abstraction.cpp  # Simple test program
```

## Key Features

### ✅ Unified Interface
- Same function calls work with both sensors
- Consistent data format (g-force units)
- Same Modbus register mapping

### ✅ Compile-Time Selection
- No runtime overhead
- Easy switching between sensors
- Clean build configuration

### ✅ Backward Compatibility
- All existing ADXL355 code preserved
- Same Modbus register layout
- Existing analytics and data processing unchanged

### ✅ Easy Integration
- Drop-in replacement for sensor-specific code
- Minimal changes to existing files
- Clean abstraction layer

## Sensor Comparison

| Feature | ADXL355 | MPU6050 |
|---------|---------|---------|
| Interface | SPI | I2C |
| Resolution | 20-bit | 16-bit |
| Noise | Ultra-low | Standard |
| Power | Very low | Low |
| Additional | Accelerometer only | 6-axis (accel + gyro) |
| Range | ±2g/±4g/±8g | ±2g/±4g/±8g/±16g |

## Usage Examples

### Basic Reading
```cpp
#include "accelerometer_interface.h"

void setup() {
    Serial.begin(115200);
    
    // Initialize (works with either sensor)
    if (!accelerometer.begin()) {
        Serial.println("Sensor init failed!");
        while(1);
    }
    
    Serial.printf("Using: %s\n", accelerometer.getSensorName());
}

void loop() {
    AccelData data;
    
    if (accelerometer.readData(data) && data.valid) {
        Serial.printf("X: %.3f g, Y: %.3f g, Z: %.3f g\n", 
                     data.x, data.y, data.z);
    }
    
    delay(100);
}
```

### Production System
The main production code automatically uses whichever sensor is configured:

```cpp
// In main.cpp - works with both sensors
if (!accelerometer.begin()) {
    Serial.println("Failed to initialize accelerometer!");
    while(1);
}

// Tasks automatically use the abstraction
// Same Modbus registers work with both sensors
```

## Configuration Details

### ADXL355 Configuration
- **Interface**: SPI (existing pins)
- **Range**: ±2g (default)
- **Features**: Ultra-low noise, high precision
- **Wiring**: Uses existing SPI connections

### MPU6050 Configuration
- **Interface**: I2C (GPIO 21=SDA, GPIO 22=SCL)
- **Range**: ±2g (configurable)
- **Features**: 6-axis sensor, gyroscope available
- **Wiring**: Requires I2C connections

## Modbus Register Mapping

**The same Modbus register map works with both sensors!**

| Register | Description | Scale | Works With |
|----------|-------------|-------|------------|
| 0-2 | Current avg X,Y,Z | ×1000 | Both sensors |
| 3-5 | Current max X,Y,Z | ×1000 | Both sensors |
| 6-8 | Current min X,Y,Z | ×1000 | Both sensors |
| 9-11 | Running avg X,Y,Z | ×1000 | Both sensors |
| ... | (all other registers) | ... | Both sensors |

## Build Configurations

### Environment Variables
You can also set the sensor selection via build flags:

**platformio.ini for ADXL355:**
```ini
[env:esp32dev_adxl355]
platform = espressif32
board = esp32dev
framework = arduino
build_flags = -DUSE_ADXL355
lib_deps = 
    SPI@2.0.0
    Wire@2.0.0
    adafruit/Adafruit MPU6050@^2.2.4
```

**platformio.ini for MPU6050:**
```ini
[env:esp32dev_mpu6050]
platform = espressif32
board = esp32dev
framework = arduino
build_flags = -DUSE_MPU6050
lib_deps = 
    SPI@2.0.0
    Wire@2.0.0
    adafruit/Adafruit MPU6050@^2.2.4
```

## Testing

### Simple Test
```bash
# Build and run the test program
pio run -e esp32dev -t upload
pio device monitor
```

### Full System Test
```bash
# Test with ADXL355
# Edit accelerometer_config.h to enable USE_ADXL355
pio run -t upload
pio device monitor

# Test with MPU6050  
# Edit accelerometer_config.h to enable USE_MPU6050
pio run -t upload
pio device monitor
```

## Troubleshooting

### Common Issues

1. **Compilation Errors**
   - Check only one sensor is defined in `accelerometer_config.h`
   - Verify all required libraries are installed

2. **ADXL355 Not Found**
   - Check SPI wiring
   - Verify CS pin configuration
   - Check power supply

3. **MPU6050 Not Found**
   - Check I2C wiring (SDA=21, SCL=22)
   - Verify I2C address (default 0x68)
   - Check power supply (3.3V)

4. **Different Data Values**
   - Both sensors are calibrated to output g-force units
   - Small differences are normal due to sensor characteristics
   - Check sensor orientation and mounting

### Debug Output

Enable debug output in config.h:
```cpp
#define ENABLE_DEBUG_OUTPUT 1
```

This will show:
- Sensor initialization details
- Raw sensor readings
- Data conversion process
- Timing information

## Migration Guide

### From ADXL355-only Code

1. Replace direct ADXL355 calls:
   ```cpp
   // Old:
   ADXL355 sensor;
   sensor.begin();
   sensor.readAcceleration(x, y, z);
   
   // New:
   accelerometer.begin();
   AccelData data;
   accelerometer.readData(data);
   float x = data.x, y = data.y, z = data.z;
   ```

2. Update includes:
   ```cpp
   // Old:
   #include "adxl355.h"
   
   // New:
   #include "accelerometer_interface.h"
   ```

3. Configure sensor selection in `accelerometer_config.h`

## Future Extensions

This abstraction makes it easy to add more accelerometers:

1. Create new implementation file (e.g., `accelerometer_lis3dh.cpp`)
2. Add sensor selection define
3. Implement the four required functions:
   - `accel_init()`
   - `accel_read()`
   - `accel_deinit()`
   - `accel_get_name()`

The same interface and Modbus registers will work automatically!
