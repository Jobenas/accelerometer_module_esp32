# Modbus RTU Register Mapping - ADXL355 Accelerometer System

## Overview
- **Slave ID**: 1
- **Baudrate**: 9600, 8N1
- **Scale Factor**: 1000 (float values multiplied by 1000 for 16-bit storage)
- **Function Codes Supported**: 0x03 (Read Holding), 0x04 (Read Input), 0x06 (Write Single), 0x10 (Write Multiple)

## Holding Registers (Function Code 0x03) - Read/Write
**Address Range**: 0-4 (5 registers total)

| Address | Name | Description | Default Value | Units |
|---------|------|-------------|---------------|-------|
| 0 | REG_DEVICE_ID | Device identification | 0x1234 (4660) | - |
| 1 | REG_FIRMWARE_VERSION | Firmware version | 100 (v1.00) | - |
| 2 | REG_SAMPLE_RATE | Sample rate | 1000 | Hz |
| 3 | REG_WINDOW_COUNT_LOW | Window count (lower 16 bits) | 0+ | count |
| 4 | REG_WINDOW_COUNT_HIGH | Window count (upper 16 bits) | 0+ | count |

### Test Commands (Holding Registers):
```bash
# Read device ID (should return 4660/0x1234)
pymodbus.console tcp --host COM_PORT --baudrate 9600 --method rtu --slave 1
> client.read_holding_registers address=0 count=1

# Read firmware version (should return 100)
> client.read_holding_registers address=1 count=1

# Read sample rate (should return 1000)
> client.read_holding_registers address=2 count=1

# Read all holding registers
> client.read_holding_registers address=0 count=5
```

## Input Registers (Function Code 0x04) - Read Only
**Address Range**: 0-23 (24 registers total)

### Current Window Statistics (0-8)
| Address | Name | Description | Scale | Range |
|---------|------|-------------|-------|-------|
| 0 | REG_CURRENT_AVG_X | Current average X | ×1000 | -32768 to +32767 |
| 1 | REG_CURRENT_AVG_Y | Current average Y | ×1000 | -32768 to +32767 |
| 2 | REG_CURRENT_AVG_Z | Current average Z | ×1000 | -32768 to +32767 |
| 3 | REG_CURRENT_MAX_X | Current maximum X | ×1000 | -32768 to +32767 |
| 4 | REG_CURRENT_MAX_Y | Current maximum Y | ×1000 | -32768 to +32767 |
| 5 | REG_CURRENT_MAX_Z | Current maximum Z | ×1000 | -32768 to +32767 |
| 6 | REG_CURRENT_MIN_X | Current minimum X | ×1000 | -32768 to +32767 |
| 7 | REG_CURRENT_MIN_Y | Current minimum Y | ×1000 | -32768 to +32767 |
| 8 | REG_CURRENT_MIN_Z | Current minimum Z | ×1000 | -32768 to +32767 |

### Running Statistics (9-17)
| Address | Name | Description | Scale | Range |
|---------|------|-------------|-------|-------|
| 9 | REG_RUNNING_AVG_X | Running average X | ×1000 | -32768 to +32767 |
| 10 | REG_RUNNING_AVG_Y | Running average Y | ×1000 | -32768 to +32767 |
| 11 | REG_RUNNING_AVG_Z | Running average Z | ×1000 | -32768 to +32767 |
| 12 | REG_GLOBAL_MAX_X | Global maximum X | ×1000 | -32768 to +32767 |
| 13 | REG_GLOBAL_MAX_Y | Global maximum Y | ×1000 | -32768 to +32767 |
| 14 | REG_GLOBAL_MAX_Z | Global maximum Z | ×1000 | -32768 to +32767 |
| 15 | REG_GLOBAL_MIN_X | Global minimum X | ×1000 | -32768 to +32767 |
| 16 | REG_GLOBAL_MIN_Y | Global minimum Y | ×1000 | -32768 to +32767 |
| 17 | REG_GLOBAL_MIN_Z | Global minimum Z | ×1000 | -32768 to +32767 |

### System Status (18-23)
| Address | Name | Description | Units |
|---------|------|-------------|-------|
| 18 | REG_TASK_STATUS | Task status flags | bitmask |
| 19 | REG_SAMPLING_ERRORS | Sampling error count | count |
| 20 | REG_PROCESSING_ERRORS | Processing error count | count |
| 21 | REG_ANALYTICS_ERRORS | Analytics error count | count |
| 22 | REG_MISSED_SAMPLES | Missed sample count | count |
| 23 | REG_LAST_UPDATE_TIME | Time since last update | ms |

### Task Status Flags (Register 18)
| Bit | Description |
|-----|-------------|
| 0 | Sampling task running |
| 1 | Processing task running |
| 2 | Analytics task running |
| 3 | Modbus task running |
| 4-15 | Reserved |

### Test Commands (Input Registers):
```bash
# Read current acceleration averages (X, Y, Z)
> client.read_input_registers address=0 count=3

# Read current acceleration max values
> client.read_input_registers address=3 count=3

# Read current acceleration min values  
> client.read_input_registers address=6 count=3

# Read running averages
> client.read_input_registers address=9 count=3

# Read global max values
> client.read_input_registers address=12 count=3

# Read global min values
> client.read_input_registers address=15 count=3

# Read system status
> client.read_input_registers address=18 count=6

# Read all input registers
> client.read_input_registers address=0 count=24
```

## Data Scaling Examples

### Accelerometer Values
- **1.000g** → stored as **1000**
- **-2.456g** → stored as **-2456**
- **0.123g** → stored as **123**

### Converting Back to Float
```python
def modbus_to_float(register_value):
    # Register values are signed 16-bit integers
    if register_value > 32767:
        register_value -= 65536  # Convert from unsigned to signed
    return register_value / 1000.0

# Example:
register_value = 1234  # From Modbus
acceleration_g = modbus_to_float(1234)  # = 1.234g
```

## Expected Behavior During Testing

### System Startup
You should see these register values immediately:
- **Holding Register 0**: 4660 (0x1234) - Device ID
- **Holding Register 1**: 100 - Firmware version
- **Holding Register 2**: 1000 - Sample rate

### With ADXL355 Connected
Input registers 0-17 will contain live accelerometer data:
- Static sensor: Z ≈ ±1000 (±1g), X,Y ≈ 0
- Moving sensor: Values change based on acceleration

### System Health
- **Register 18**: Should show task status bits set (value > 0)
- **Registers 19-22**: Error counts (should remain low/zero)
- **Register 23**: Time since last update (should be small, <1000ms)

## Quick Test Sequence

1. **Verify Device**: Read holding register 0 → should return 4660
2. **Check Firmware**: Read holding register 1 → should return 100  
3. **Read Live Data**: Read input registers 0-2 → should show current X,Y,Z acceleration
4. **Monitor Changes**: Read input registers repeatedly to see data updates
5. **Check System Health**: Read input registers 18-23 for status and errors

Your successful test `[DATA] 1234` confirms the device ID register is working perfectly!
