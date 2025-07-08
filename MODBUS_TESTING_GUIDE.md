# ADXL355 ESP32 Modbus Interface Testing Guide

This document provides instructions for testing the Modbus RTU interface of the ADXL355 ESP32 accelerometer project.

## Project Overview

The ESP32 firmware implements:
- 1kHz accelerometer sampling using ADXL355
- Real-time analytics (average, min, max per window)
- FreeRTOS-based modular architecture
- **Modbus RTU slave interface** for remote access to analytics data

## Hardware Setup

### ESP32 Connections
- **ADXL355 Sensor**: SPI interface (CS, MOSI, MISO, SCK)
- **Modbus RTU**: Serial2 interface
  - TX: Pin 17
  - RX: Pin 16
  - DE/RE: Pin 4 (Driver Enable/Receiver Enable for RS485)

### For RS485 Communication (Recommended)
1. Connect an RS485 transceiver (e.g., MAX485) to ESP32:
   - ESP32 Pin 17 (TX) → RS485 DI (Data Input)
   - ESP32 Pin 16 (RX) → RS485 RO (Receiver Output)
   - ESP32 Pin 4 → RS485 DE/RE (Direction control)
   - Connect A and B lines to your Modbus master device

### For Direct Serial Testing (Simple)
1. Connect ESP32 directly to another ESP32 or USB-Serial adapter:
   - ESP32 Pin 17 (TX) → Master RX
   - ESP32 Pin 16 (RX) → Master TX
   - GND → GND

## Modbus Configuration

- **Slave ID**: 1
- **Baud Rate**: 9600
- **Parity**: None
- **Stop Bits**: 1
- **Data Bits**: 8

## Register Map

### Holding Registers (Read/Write) - Addresses 0-4
| Address | Name | Description | Units |
|---------|------|-------------|-------|
| 0 | Device ID | 0x3355 (ADXL355) | - |
| 1 | Firmware Version | Version × 100 | v1.00 = 100 |
| 2 | Sample Rate | Sampling frequency | Hz |
| 3 | Window Count Low | Lower 16 bits of window count | - |
| 4 | Window Count High | Upper 16 bits of window count | - |

### Input Registers (Read-Only) - Addresses 5-28
**Note**: Due to library limitations, all registers are accessed as holding registers with combined addressing.

#### Current Window Statistics (5-13)
| Address | Name | Description | Scale |
|---------|------|-------------|-------|
| 5 | Current Avg X | Average X for current 1-second window | ÷1000 |
| 6 | Current Avg Y | Average Y for current 1-second window | ÷1000 |
| 7 | Current Avg Z | Average Z for current 1-second window | ÷1000 |
| 8 | Current Max X | Maximum X for current window | ÷1000 |
| 9 | Current Max Y | Maximum Y for current window | ÷1000 |
| 10 | Current Max Z | Maximum Z for current window | ÷1000 |
| 11 | Current Min X | Minimum X for current window | ÷1000 |
| 12 | Current Min Y | Minimum Y for current window | ÷1000 |
| 13 | Current Min Z | Minimum Z for current window | ÷1000 |

#### Running Statistics (14-22)
| Address | Name | Description | Scale |
|---------|------|-------------|-------|
| 14 | Running Avg X | Running average X | ÷1000 |
| 15 | Running Avg Y | Running average Y | ÷1000 |
| 16 | Running Avg Z | Running average Z | ÷1000 |
| 17 | Global Max X | Global maximum X since startup | ÷1000 |
| 18 | Global Max Y | Global maximum Y since startup | ÷1000 |
| 19 | Global Max Z | Global maximum Z since startup | ÷1000 |
| 20 | Global Min X | Global minimum X since startup | ÷1000 |
| 21 | Global Min Y | Global minimum Y since startup | ÷1000 |
| 22 | Global Min Z | Global minimum Z since startup | ÷1000 |

#### System Status (23-28)
| Address | Name | Description | Format |
|---------|------|-------------|--------|
| 23 | Task Status | FreeRTOS task status flags | Bit field |
| 24 | Sampling Errors | Sampling task error count | Count |
| 25 | Processing Errors | Processing task error count | Count |
| 26 | Analytics Errors | Analytics task error count | Count |
| 27 | Missed Samples | Missed sample count | Count |
| 28 | Last Update Time | Time since last analytics update | ms |

### Task Status Bit Field (Register 23)
| Bit | Description |
|-----|-------------|
| 0 | Sampling Task Running |
| 1 | Processing Task Running |
| 2 | Analytics Task Running |
| 3 | Modbus Task Running |

## Testing Methods

### Method 1: Arduino Modbus Master Test

Use the included `modbus_master_test.ino` sketch:

1. Upload the main firmware to the first ESP32
2. Upload `modbus_master_test.ino` to a second ESP32
3. Connect the two ESP32s as described in Hardware Setup
4. Open Serial Monitor for the master ESP32
5. Observe Modbus communication every 5 seconds

### Method 2: Python pymodbus (Requires PC with RS485 adapter)

1. Install Python dependencies:
   ```bash
   pip install pymodbus pyserial
   ```

2. Update `COM_PORT` in `test_modbus_master.py` to match your RS485 adapter

3. Run the test:
   ```bash
   python test_modbus_master.py
   ```

### Method 3: Modbus Poll/Similar Tools

Use commercial Modbus tools like:
- Modbus Poll (Windows)
- QModMaster (Cross-platform)
- ModbusMechanic (Online)

Configuration:
- Connection: Serial RTU
- Slave ID: 1
- Baud: 9600, N, 8, 1
- Read holding registers 0-28

## Typical Register Values

### Normal Operation
- **Device ID (0)**: 0x3355
- **Firmware Version (1)**: 100 (v1.00)
- **Sample Rate (2)**: 1000 Hz
- **Task Status (23)**: 0x000F (all tasks running)
- **Error Counts (24-27)**: Should be 0 or very low
- **Accelerometer Values**: Depend on sensor orientation and vibration

### Example Reading
```
Holding Registers:
0: 0x3355 (Device ID)
1: 100 (Firmware v1.00)
2: 1000 (Sample rate)
3: 0x1234 (Window count low)
4: 0x0000 (Window count high)

Analytics Data:
5-7: Current averages (scaled by 1000)
8-10: Current maximums
11-13: Current minimums
14-22: Running statistics
23: 0x000F (All tasks running)
24-28: Error counts and timing
```

## Troubleshooting

### No Modbus Response
1. Check wiring and connections
2. Verify baud rate (9600)
3. Confirm slave ID (1)
4. Check RS485 transceiver direction control
5. Monitor ESP32 Serial output for initialization messages

### Wrong Data Values
1. Check register addressing (combined holding register space)
2. Apply correct scaling (÷1000 for accelerometer values)
3. Handle signed 16-bit integers correctly
4. Verify window count is incrementing

### Task Status Issues
1. Check bit 0-3 in register 23
2. Monitor error counts in registers 24-27
3. Review ESP32 Serial output for task failures

## Integration Examples

### PLC Integration
Most PLCs support Modbus RTU. Configure:
- Serial port parameters
- Slave device (ID 1)
- Register mapping
- Scaling factors

### SCADA Integration
Configure Modbus RTU driver with:
- Device address: 1
- Register ranges: 0-28
- Appropriate data types (16-bit signed/unsigned)
- Scaling for accelerometer data

### Python Automation
```python
from pymodbus.client import ModbusSerialClient

client = ModbusSerialClient(port='COM3', baudrate=9600)
result = client.read_holding_registers(5, 9, slave=1)  # Current analytics
if not result.isError():
    avg_x = (result.registers[0] if result.registers[0] < 32768 
             else result.registers[0] - 65536) / 1000.0
    print(f"Current Average X: {avg_x:.3f}")
```

## Data Logging

The Modbus interface enables:
- Continuous monitoring of accelerometer statistics
- Remote data collection without USB connection
- Integration with industrial monitoring systems
- Real-time alerting based on vibration thresholds

## Performance Notes

- Modbus task runs at 100Hz (10ms update cycle)
- Register updates occur every 100ms
- Analytics windows update every 1 second
- No significant impact on 1kHz sampling performance
- Combined register addressing for library compatibility
