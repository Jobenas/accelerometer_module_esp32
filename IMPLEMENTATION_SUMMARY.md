# Accelerometer Abstraction Implementation Summary

## ✅ Successfully Implemented

I have successfully refactored your ADXL355 accelerometer project to support both ADXL355 and MPU6050 sensors through a clean abstraction layer.

## 📁 Files Created/Modified

### New Files Created:
1. **`include/accelerometer_config.h`** - Sensor selection and common interface
2. **`include/accelerometer_interface.h`** - High-level wrapper class  
3. **`src/accelerometer_adxl355.cpp`** - ADXL355 implementation
4. **`src/accelerometer_mpu6050.cpp`** - MPU6050 implementation
5. **`src/accelerometer_interface.cpp`** - Wrapper implementation
6. **`test/test_accelerometer_abstraction.cpp`** - Simple test program
7. **`ACCELEROMETER_ABSTRACTION_README.md`** - Complete documentation

### Modified Files:
1. **`platformio.ini`** - Added MPU6050 library dependency
2. **`src/main.cpp`** - Updated to use accelerometer abstraction
3. **`src/task_manager.cpp`** - Updated to use accelerometer abstraction

## 🎯 Key Features

### ✅ Compile-Time Sensor Selection
Switch between sensors by editing `include/accelerometer_config.h`:

```cpp
// For ADXL355:
#define USE_ADXL355
// #define USE_MPU6050

// For MPU6050:
// #define USE_ADXL355
#define USE_MPU6050
```

### ✅ Unified Interface
Both sensors use the same function calls:
```cpp
accelerometer.begin();          // Initialize either sensor
AccelData data;
accelerometer.readData(data);   // Read from either sensor
```

### ✅ Same Modbus Register Map
- All existing Modbus registers work with both sensors
- Same scaling factors (×1000)
- Same data ranges and formats
- Your device configuration utility works unchanged

### ✅ Backward Compatibility
- All original ADXL355 code preserved
- Existing analytics and data processing unchanged
- Same FreeRTOS task structure

## 🔧 How to Use

### Step 1: Choose Your Sensor
Edit `include/accelerometer_config.h` to select ADXL355 or MPU6050

### Step 2: Wire Your Sensor

**ADXL355 (SPI):**
- Uses existing SPI connections
- Same pins as before

**MPU6050 (I2C):**
- SDA → GPIO 21
- SCL → GPIO 22  
- VCC → 3.3V
- GND → GND

### Step 3: Build and Flash
```bash
pio run -t upload
pio device monitor
```

## 📊 Sensor Comparison

| Feature | ADXL355 | MPU6050 |
|---------|---------|---------|
| **Interface** | SPI | I2C |
| **Resolution** | 20-bit | 16-bit |
| **Noise** | Ultra-low | Standard |
| **Range** | ±2g/±4g/±8g | ±2g/±4g/±8g/±16g |
| **Additional** | Accelerometer only | 6-axis (accel + gyro) |
| **Power** | Very low | Low |

## 🔬 Testing

### Build Status: ✅ SUCCESS
- Project compiles successfully
- All dependencies resolved
- No compilation errors
- Memory usage: RAM 6.9%, Flash 22.2%

### Ready for Testing:
1. **ADXL355 Test**: Keep current configuration, build and test
2. **MPU6050 Test**: Change config to MPU6050, build and test
3. **Modbus Test**: Same register map works with both sensors

## 📋 Modbus Register Map (Unchanged)

Your device configuration utility can use the exact same settings:

| Register | Description | Function Code | Data Type |
|----------|-------------|---------------|-----------|
| 0-2 | Current avg X,Y,Z | 0x04 | int16 |
| 3-5 | Current max X,Y,Z | 0x04 | int16 |
| 6-8 | Current min X,Y,Z | 0x04 | int16 |
| 9-11 | Running avg X,Y,Z | 0x04 | int16 |
| 12-17 | Global max/min | 0x04 | int16 |
| 18-23 | System status | 0x04 | int16 |

**Scale Factor**: All acceleration values multiplied by 1000

## 🚀 Next Steps

1. **Test with ADXL355**: Verify existing functionality works
2. **Test with MPU6050**: Wire up MPU6050 and test
3. **Validate Modbus**: Confirm your device utility reads both sensors
4. **Production Use**: Choose the sensor that best fits your needs

## 💡 Benefits Achieved

- ✅ **Single codebase** supports both accelerometers
- ✅ **Easy switching** via simple configuration change  
- ✅ **Same Modbus interface** - no changes needed to your device utility
- ✅ **Future-proof** - easy to add more accelerometers later
- ✅ **Clean architecture** - sensor-specific code isolated
- ✅ **Maintained compatibility** - existing features preserved

The refactoring is complete and ready for testing! You can now easily switch between ADXL355 and MPU6050 accelerometers while maintaining full compatibility with your existing Modbus device configuration.
