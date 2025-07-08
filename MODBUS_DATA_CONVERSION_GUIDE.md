# ADXL355 ESP32 Accelerometer - Data Conversion & Modbus Register Documentation

## **System Overview**

This document describes the complete data flow from the ADXL355 accelerometer through to the Modbus RTU interface, ensuring consistent integer-based conversions throughout the system.

## **Data Flow Pipeline**

```
ADXL355 Sensor (20-bit raw) → Analytics (g-values) → Modbus (scaled integers)
     ±524,288              →      ±2g            →    ±2000 (max)
```

## **Conversion Stages**

### **Stage 1: ADXL355 Raw Data → G-Values**
- **Input**: 20-bit signed raw values from ADXL355 (range: ±524,288)
- **Scale Factor**: `256,000` (ADXL355_SCALE_FACTOR)
- **Output**: Acceleration in g-units (range: ±2.048g)
- **Location**: `Analytics::processBufferStats()` in `analytics.cpp`

**Formula**: `g_value = raw_value / 256000.0f`

**Example**:
- Raw: `-252,316` → G-value: `-0.985609g`
- Raw: `6,121` → G-value: `0.023910g`
- Raw: `-25,921` → G-value: `-0.101254g`

### **Stage 2: G-Values → Modbus Integer Registers**
- **Input**: Acceleration in g-units (±2g typical range)
- **Scale Factor**: `1,000` (MODBUS_SCALE_FACTOR)
- **Output**: 16-bit signed integers (range: ±32,767)
- **Location**: `ModbusRTUCustom::floatToScaledInt()` in `modbus_rtu_custom.cpp`

**Formula**: `modbus_value = (int16_t)(g_value * 1000.0f)`

**Example**:
- G-value: `-0.985609g` → Modbus: `-986`
- G-value: `0.023910g` → Modbus: `24`
- G-value: `-0.101254g` → Modbus: `-101`

## **Modbus Register Map**

### **Input Registers (Function Code 0x04) - Read Only**

All acceleration values are **scaled by 1000** and stored as **signed 16-bit integers**.

#### **Current Window Statistics (Addresses 0-14)**
| Address | Register Name | Description | Units | Range | Example |
|---------|---------------|-------------|-------|-------|---------|
| 0 | `REG_CURRENT_AVG_X` | Current average X acceleration | mg | ±32,767 | -986 (-0.986g) |
| 1 | `REG_CURRENT_AVG_Y` | Current average Y acceleration | mg | ±32,767 | 24 (0.024g) |
| 2 | `REG_CURRENT_AVG_Z` | Current average Z acceleration | mg | ±32,767 | -101 (-0.101g) |
| 3 | `REG_CURRENT_MAX_X` | Current maximum X acceleration | mg | ±32,767 | -950 (-0.950g) |
| 4 | `REG_CURRENT_MAX_Y` | Current maximum Y acceleration | mg | ±32,767 | 50 (0.050g) |
| 5 | `REG_CURRENT_MAX_Z` | Current maximum Z acceleration | mg | ±32,767 | -80 (-0.080g) |
| 6 | `REG_CURRENT_MIN_X` | Current minimum X acceleration | mg | ±32,767 | -1020 (-1.020g) |
| 7 | `REG_CURRENT_MIN_Y` | Current minimum Y acceleration | mg | ±32,767 | -10 (-0.010g) |
| 8 | `REG_CURRENT_MIN_Z` | Current minimum Z acceleration | mg | ±32,767 | -120 (-0.120g) |
| 9 | `REG_CURRENT_STD_X` | Current standard deviation X | mg | ±32,767 | 35 (0.035g) |
| 10 | `REG_CURRENT_STD_Y` | Current standard deviation Y | mg | ±32,767 | 15 (0.015g) |
| 11 | `REG_CURRENT_STD_Z` | Current standard deviation Z | mg | ±32,767 | 20 (0.020g) |
| 12 | `REG_CURRENT_RMS_X` | Current RMS X acceleration | mg | ±32,767 | 987 (0.987g) |
| 13 | `REG_CURRENT_RMS_Y` | Current RMS Y acceleration | mg | ±32,767 | 28 (0.028g) |
| 14 | `REG_CURRENT_RMS_Z` | Current RMS Z acceleration | mg | ±32,767 | 103 (0.103g) |

#### **Running Statistics (Addresses 15-29)**
| Address | Register Name | Description | Units | Range |
|---------|---------------|-------------|-------|-------|
| 15 | `REG_RUNNING_AVG_X` | Running average X acceleration | mg | ±32,767 |
| 16 | `REG_RUNNING_AVG_Y` | Running average Y acceleration | mg | ±32,767 |
| 17 | `REG_RUNNING_AVG_Z` | Running average Z acceleration | mg | ±32,767 |
| 18 | `REG_RUNNING_STD_X` | Running standard deviation X | mg | ±32,767 |
| 19 | `REG_RUNNING_STD_Y` | Running standard deviation Y | mg | ±32,767 |
| 20 | `REG_RUNNING_STD_Z` | Running standard deviation Z | mg | ±32,767 |
| 21 | `REG_RUNNING_RMS_X` | Running RMS X acceleration | mg | ±32,767 |
| 22 | `REG_RUNNING_RMS_Y` | Running RMS Y acceleration | mg | ±32,767 |
| 23 | `REG_RUNNING_RMS_Z` | Running RMS Z acceleration | mg | ±32,767 |
| 24 | `REG_GLOBAL_MAX_X` | Global maximum X since startup | mg | ±32,767 |
| 25 | `REG_GLOBAL_MAX_Y` | Global maximum Y since startup | mg | ±32,767 |
| 26 | `REG_GLOBAL_MAX_Z` | Global maximum Z since startup | mg | ±32,767 |
| 27 | `REG_GLOBAL_MIN_X` | Global minimum X since startup | mg | ±32,767 |
| 28 | `REG_GLOBAL_MIN_Y` | Global minimum Y since startup | mg | ±32,767 |
| 29 | `REG_GLOBAL_MIN_Z` | Global minimum Z since startup | mg | ±32,767 |

#### **System Status (Addresses 30-35)**
| Address | Register Name | Description | Units | Format |
|---------|---------------|-------------|-------|--------|
| 30 | `REG_TASK_STATUS` | FreeRTOS task status flags | - | Bit field |
| 31 | `REG_SAMPLING_ERRORS` | Sampling error count | count | Unsigned 16-bit |
| 32 | `REG_PROCESSING_ERRORS` | Processing error count | count | Unsigned 16-bit |
| 33 | `REG_ANALYTICS_ERRORS` | Analytics error count | count | Unsigned 16-bit |
| 34 | `REG_MISSED_SAMPLES` | Missed sample count | count | Unsigned 16-bit |
| 35 | `REG_LAST_UPDATE_TIME` | Time since last update | ms | Unsigned 16-bit |

### **Holding Registers (Function Code 0x03) - Read/Write**

| Address | Register Name | Description | Units | Default |
|---------|---------------|-------------|-------|---------|
| 0 | `REG_DEVICE_ID` | Device identification | - | 0x1234 |
| 1 | `REG_FIRMWARE_VERSION` | Firmware version × 100 | - | 100 (v1.00) |
| 2 | `REG_SAMPLE_RATE` | Sampling frequency | Hz | 1000 |
| 3 | `REG_WINDOW_COUNT_LOW` | Window count (lower 16 bits) | count | 0+ |
| 4 | `REG_WINDOW_COUNT_HIGH` | Window count (upper 16 bits) | count | 0+ |

## **Data Conversion Functions**

### **Client-Side Conversion (Reading Modbus Values)**

```python
def modbus_to_acceleration_g(modbus_value):
    """Convert Modbus register value back to acceleration in g-units"""
    # Handle signed 16-bit conversion
    if modbus_value > 32767:
        modbus_value -= 65536
    return modbus_value / 1000.0

# Example usage:
x_raw = 0xFC27  # -987 in signed 16-bit
x_acceleration_g = modbus_to_acceleration_g(x_raw)  # -0.987g
```

```c
// C/C++ conversion
float modbus_to_g(int16_t modbus_value) {
    return (float)modbus_value / 1000.0f;
}
```

### **Data Validation**

**Expected Value Ranges**:
- **Static sensor**: Z ≈ ±1000 (±1g gravity), X,Y ≈ 0
- **Moving sensor**: Values change based on acceleration/vibration
- **Maximum theoretical**: ±2048 (±2.048g) for ADXL355 ±2g range
- **Practical maximum**: ±32767 (±32.767g) due to 16-bit register limit

## **Configuration Constants**

```c
// In modbus_rtu_custom.h
#define MODBUS_SCALE_FACTOR     1000  // Scale factor for float→integer conversion
#define NUM_INPUT_REGISTERS     36    // Total input registers (updated for STD/RMS)
#define NUM_HOLDING_REGISTERS   5     // Total holding registers

// In analytics.cpp
const float ADXL355_SCALE_FACTOR = 256000.0f;  // ADXL355 raw→g conversion
```

## **Example Modbus Transactions**

### **Reading Current Standard Deviation & RMS**
```
Request:  [01] [04] [00 09] [00 06]  // Read STD & RMS registers 9-14
Response: [01] [04] [0C] [00 01] [00 00] [00 00] [03 D8] [00 0E] [00 6A]
Decoded:  STD: X=0.001g, Y=0.000g, Z=0.000g | RMS: X=0.984g, Y=0.014g, Z=0.106g
```

### **Reading System Status**
```
Request:  [01] [04] [00 1E] [00 01]  // Read task status register (30)
Response: [01] [04] [02] [00 0F]     // All tasks running (0x000F)
```

## **Debugging & Validation**

### **Serial Monitor Output**
```
[ANALYTICS] Raw: X=-252316.0, Y=6121.0, Z=-25921.0 -> G: X=-0.985609, Y=0.023910, Z=-0.101254
[ANALYTICS] STD: X=0.001000, Y=0.000000, Z=0.000000 | RMS: X=0.984000, Y=0.014000, Z=0.106000
[Modbus-DEBUG] Analytics data (g-values) - X: -0.985188, Y: 0.023871, Z: -0.100351
[Modbus-DEBUG] STD values (g-values) - X: 0.001000, Y: 0.000000, Z: 0.000000
[Modbus-DEBUG] RMS values (g-values) - X: 0.984000, Y: 0.014000, Z: 0.106000
[Modbus-DEBUG] Scaled for Modbus: X=-985, Y=23, Z=-100 (should be in ±32767 range)
```

### **Data Integrity Checks**
1. **No clamping messages**: Indicates values are within ±32767 range
2. **Reasonable g-values**: Typically ±2g for normal operation
3. **Consistent scaling**: Raw/256000 ≈ Modbus/1000

## **Summary**

✅ **All registers use consistent integer values**  
✅ **Two-stage conversion with documented scale factors**  
✅ **Signed 16-bit integers for acceleration data**  
✅ **1mg resolution (1/1000 g)**  
✅ **Standard deviation and RMS analytics implemented**  
✅ **36 total Modbus registers (12 new STD/RMS registers)**  
✅ **No data clamping under normal operation**  
✅ **Full traceability from sensor to Modbus**  
✅ **Mathematical accuracy verified (STD ≈ 0 for static sensor)**  

The enhanced system provides **comprehensive vibration analysis** with 1 milligram resolution across all parameters, supporting both basic statistics and advanced analytics for condition monitoring applications.
