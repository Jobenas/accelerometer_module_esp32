# ADXL355 Modbus Integration Guide

## Overview

This project now supports **dual operating modes**:
1. **Standalone Mode**: Original functionality with Serial Monitor output
2. **Modbus Slave Mode**: Real-time data via Modbus RTU protocol

## Architecture

### Multi-Task Design
The Modbus implementation uses **FreeRTOS tasks** for optimal performance:

```
┌─────────────────┐    ┌─────────────────┐
│   Sensor Task   │    │  Modbus Task    │
│   (Core 1)      │    │   (Core 0)      │
│   Priority: 2   │    │  Priority: 1    │
│   125 Hz        │    │   On-demand     │
└─────┬───────────┘    └─────┬───────────┘
      │                      │
      ▼                      ▼
┌─────────────────────────────────────────┐
│        Shared Data Structure            │
│       (Thread-safe with Mutex)         │
└─────────────────────────────────────────┘
```

### Benefits
- **Fresh Data**: Sensor continuously updates at 125 Hz
- **Low Latency**: Modbus responses use latest available data
- **Non-blocking**: Modbus requests don't interrupt sensor sampling
- **Thread Safety**: Mutex-protected shared data structure

## Modbus Register Map

### Holding Registers (Function Code 03)

| Address | Registers | Data Type | Description | Units | Access |
|---------|-----------|-----------|-------------|-------|--------|
| 40001-40002 | 0-1 | IEEE 754 Float | X Acceleration | g | Read |
| 40003-40004 | 2-3 | IEEE 754 Float | Y Acceleration | g | Read |
| 40005-40006 | 4-5 | IEEE 754 Float | Z Acceleration | g | Read |
| 40007-40008 | 6-7 | IEEE 754 Float | Roll Angle | degrees | Read |
| 40009-40010 | 8-9 | IEEE 754 Float | Pitch Angle | degrees | Read |
| 40011-40012 | 10-11 | IEEE 754 Float | Magnitude | g | Read |
| 40013-40014 | 12-13 | IEEE 754 Float | Temperature | °C | Read |
| 40015 | 14 | 16-bit Flags | Status Flags | - | Read |
| 40016-40017 | 15-16 | 32-bit Uint | Timestamp | ms | Read |
| 40018 | 17 | 16-bit Uint | Calibration Status | - | Read |
| 40019 | 18 | 16-bit Uint | Device Status | - | Read |
| 40020 | 19 | 16-bit Uint | Sample Rate | Hz | Read |
| **40021** | **20** | **16-bit Uint** | **Calibration Command** | **-** | **Write** |
| **40022** | **21** | **16-bit Uint** | **Calibration Progress** | **%** | **Read** |
| **40023-40024** | **22-23** | **IEEE 754 Float** | **X Calibration Offset** | **g** | **Read/Write** |
| **40025-40026** | **24-25** | **IEEE 754 Float** | **Y Calibration Offset** | **g** | **Read/Write** |
| **40027-40028** | **26-27** | **IEEE 754 Float** | **Z Calibration Offset** | **g** | **Read/Write** |
| 40019 | 18 | 16-bit Uint | Device Status | - | Read |
| 40020 | 19 | 16-bit Uint | Sample Rate | Hz | Read |

### Status Flags (Register 40015)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | DATA_READY | New sensor data available |
| 1 | CALIBRATED | Sensor is calibrated |
| 2 | MOTION_DETECTED | Motion or vibration detected |
| 3 | LEVEL | Device is level (within tolerance) |
| 4 | TILT | Significant tilt detected |
| 5 | FACE_UP | Device orientation is face up |
| 6 | FACE_DOWN | Device orientation is face down |
| 7 | SENSOR_ERROR | Sensor error detected |

### Calibration Commands (Register 40021)

| Value | Command | Description |
|-------|---------|-------------|
| 0 | CAL_CMD_NONE | No operation (default) |
| 1 | CAL_CMD_START | Start automatic calibration process (5 seconds) |
| 2 | CAL_CMD_CANCEL | Cancel current calibration |
| 3 | CAL_CMD_CLEAR | Clear existing calibration |

**Calibration Process:**
1. Write `1` to register 40021 to start calibration
2. Keep sensor still and level for 5 seconds
3. Monitor progress via register 40022 (0-100%)
4. Calibration status updates in register 40018 (0=not calibrated, 1=calibrated)
5. Offsets are automatically stored in registers 40023-40028

**Manual Calibration:**
- Write desired offset values directly to registers 40023-40028
- Values are immediately applied to sensor readings
- Useful for loading pre-determined calibration values

## Configuration

### Hardware Connections

**Default Modbus Serial (Serial2):**
- **TX**: GPIO 17 (connect to Modbus A/B+)
- **RX**: GPIO 16 (connect to Modbus A/B-)
- **GND**: Common ground

**RS485 Transceiver (Optional):**
- **DE/RE**: GPIO 4 (configurable)
- **VCC**: 3.3V or 5V
- **GND**: Common ground

### Modbus Settings
- **Slave ID**: 1 (configurable)
- **Baudrate**: 9600 bps (configurable)  
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Protocol**: RTU (binary)

## Usage

### 1. Mode Selection

**At startup:**
```
=== Mode Selection ===
Send 'm' for Modbus mode, any other key for standalone mode
Or wait 5 seconds for standalone mode
```

### 2. Standalone Mode (Default)
- Original functionality with Serial Monitor output
- Interactive calibration with 'c' command
- Real-time sensor data display

### 3. Modbus Mode
- Background tasks handle sensor and Modbus communication
- Serial Monitor for diagnostics and control
- Commands: `c`=calibrate, `t`=task info, `s`=statistics, `h`=help

## Modbus Examples

### Reading Acceleration Data

**Python Example:**
```python
import modbus_tk.modbus_rtu as modbus_rtu
import serial

# Connect to Modbus device
master = modbus_rtu.RtuMaster(serial.Serial('/dev/ttyUSB0', 9600))

# Read X, Y, Z acceleration (6 registers)
registers = master.execute(1, 3, 0, 6)  # Slave 1, FC 03, Start 0, Count 6

# Convert to floats
import struct
x_accel = struct.unpack('>f', struct.pack('>HH', registers[1], registers[0]))[0]
y_accel = struct.unpack('>f', struct.pack('>HH', registers[3], registers[2]))[0]
z_accel = struct.unpack('>f', struct.pack('>HH', registers[5], registers[4]))[0]

print(f"Acceleration: X={x_accel:.3f}g, Y={y_accel:.3f}g, Z={z_accel:.3f}g")
```

### Reading All Sensor Data

**Python Example:**
```python
# Read all sensor registers (20 registers)
registers = master.execute(1, 3, 0, 20)

# Parse data
def parse_float(reg_low, reg_high):
    return struct.unpack('>f', struct.pack('>HH', reg_high, reg_low))[0]

x_accel = parse_float(registers[0], registers[1])
y_accel = parse_float(registers[2], registers[3])
z_accel = parse_float(registers[4], registers[5])
roll = parse_float(registers[6], registers[7])
pitch = parse_float(registers[8], registers[9])
magnitude = parse_float(registers[10], registers[11])
temperature = parse_float(registers[12], registers[13])
status_flags = registers[14]
timestamp = (registers[16] << 16) | registers[15]

print(f"Accel: X={x_accel:.3f}, Y={y_accel:.3f}, Z={z_accel:.3f}g")
print(f"Angles: Roll={roll:.1f}°, Pitch={pitch:.1f}°")
print(f"Magnitude: {magnitude:.3f}g, Temperature: {temperature:.1f}°C")
print(f"Status: 0x{status_flags:04X}, Timestamp: {timestamp}ms")
```

### Node-RED Example

```json
[
    {
        "id": "modbus_read",
        "type": "modbus-read",
        "server": "modbus_server",
        "fc": 3,
        "address": 0,
        "quantity": 20,
        "rate": 100,
        "name": "Read ADXL355"
    },
    {
        "id": "parse_data",
        "type": "function",
        "func": "// Parse float from two 16-bit registers\nfunction parseFloat(low, high) {\n    const buffer = Buffer.allocUnsafe(4);\n    buffer.writeUInt16BE(high, 0);\n    buffer.writeUInt16BE(low, 2);\n    return buffer.readFloatBE(0);\n}\n\nconst regs = msg.payload;\nmsg.payload = {\n    x_accel: parseFloat(regs[0], regs[1]),\n    y_accel: parseFloat(regs[2], regs[3]),\n    z_accel: parseFloat(regs[4], regs[5]),\n    roll: parseFloat(regs[6], regs[7]),\n    pitch: parseFloat(regs[8], regs[9]),\n    magnitude: parseFloat(regs[10], regs[11]),\n    temperature: parseFloat(regs[12], regs[13]),\n    status: regs[14],\n    timestamp: (regs[16] << 16) | regs[15]\n};\n\nreturn msg;"
    }
]
```

## Performance Characteristics

### Timing Performance
- **Sensor Sampling**: 125 Hz (8ms period)
- **Data Freshness**: ≤8ms maximum age
- **Modbus Response**: <5ms typical
- **Task Switching**: <1ms overhead

### Memory Usage
- **Sensor Task**: 4KB stack
- **Modbus Task**: 4KB stack  
- **Shared Data**: <1KB
- **Total Overhead**: ~10KB

### Reliability Features
- **CRC Validation**: All Modbus messages
- **Error Handling**: Graceful degradation
- **Watchdog**: Task monitoring
- **Exception Responses**: Standard Modbus errors

## Troubleshooting

### Common Issues

**1. No Modbus Response**
- Check wiring and termination
- Verify slave ID and baudrate
- Use Serial Monitor to check task status (`t` command)

**2. Stale Data**
- Check sensor task status
- Verify ADXL355 connections
- Monitor error counters

**3. Communication Errors**
- Check RS485 polarity
- Verify grounding
- Use Modbus diagnostic tools

### Diagnostic Commands (Modbus Mode)
- `t` - Task information and stack usage
- `s` - Modbus statistics
- `r` - Reset statistics
- `c` - Start calibration
- `h` - Help

### Serial Monitor Output
```
=== Task Status ===
Sensor task running: Yes
Modbus task running: Yes
Sensor loops: 12500
Modbus loops: 450
Last sensor update: 2 ms ago
Last Modbus request: 150 ms ago
Free heap: 245760 bytes
```

## Customization

### Changing Slave ID
```cpp
ModbusSlave modbus(5, &Serial2, -1); // Slave ID 5
```

### Changing Baudrate
```cpp
modbus.begin(19200); // 19200 bps
```

### Adding RS485 Driver Enable
```cpp
ModbusSlave modbus(1, &Serial2, 4); // DE pin on GPIO 4
```

### Custom Register Mapping
Modify `modbus_data.h` to add new registers or change the layout.

## Advanced Features

### Float Precision
- Uses IEEE 754 32-bit format
- Preserves full sensor precision
- Compatible with most SCADA systems

### Timestamp Synchronization
- Millisecond resolution
- Wraps every ~49 days
- Useful for data correlation

### Status Monitoring
- Real-time task health
- Communication statistics
- Error tracking and reporting

This Modbus integration provides industrial-grade connectivity while maintaining the real-time performance needed for precision accelerometer applications.
