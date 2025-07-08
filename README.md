# ADXL355 Accelerometer - Modular ESP32 Project

## Overview
This project provides a comprehensive and modular implementation for interfacing with the ADXL355 accelerometer using an ESP32. The code has been refactored from a monolithic structure into separate, reusable modules for better maintainability and organization.

## Project Structure

```
src/
├── main.cpp              # High-level application logic and main loop
├── config.h              # Hardware pin definitions and register constants
├── adxl355.h/.cpp        # ADXL355 sensor class implementation
├── angle_calculator.h/.cpp # Angle calculation and filtering logic
└── calibration.h/.cpp    # Calibration system implementation
```

## Features

### Core Functionality
- **Robust SPI Communication**: Reliable data transfer with proper power sequencing
- **Device Validation**: Automatic detection and validation of ADXL355 sensor
- **Multiple Measurement Ranges**: Support for ±2g, ±4g, and ±8g ranges
- **Temperature Reading**: Built-in temperature sensor access

### Advanced Features
- **Angle Calculation**: Roll and pitch angle computation from acceleration data
- **Low-Pass Filtering**: Smooth angle readings with configurable filter strength
- **Orientation Detection**: Automatic detection of device orientation (Face Up/Down, etc.)
- **Calibration System**: Interactive zero-level calibration for improved accuracy
- **Motion Detection**: Automatic detection of motion and vibration
- **Level Detection**: Determine if device is level within configurable tolerance

### Diagnostic Features
- **Status Monitoring**: Real-time sensor status and health monitoring
- **Error Detection**: Comprehensive error checking and reporting
- **Debug Output**: Detailed diagnostic information for troubleshooting

## Module Descriptions

### ADXL355 Class (`adxl355.h/.cpp`)
Handles all sensor-specific operations:
- SPI communication and register access
- Device initialization and configuration
- Data reading and conversion
- Temperature measurement
- Status monitoring and diagnostics

### AngleCalculator Class (`angle_calculator.h/.cpp`)
Manages angle-related calculations:
- Roll and pitch angle computation
- Low-pass filtering for smooth readings
- Magnitude calculation
- Orientation classification
- Motion and tilt detection

### Calibration Class (`calibration.h/.cpp`)
Provides calibration functionality:
- Interactive calibration process
- Offset calculation and storage
- Calibration status tracking
- Real-time calibration application

### Configuration (`config.h`)
Centralizes all hardware and register definitions:
- GPIO pin assignments
- ADXL355 register addresses
- Range and filter settings
- Device identification constants

## Usage

### Basic Operation
1. Connect the ADXL355 to your ESP32 according to the pin definitions in `config.h`
2. Upload the firmware to your ESP32
3. Open the Serial Monitor at 115200 baud
4. The sensor will initialize automatically and begin streaming data

### Calibration
- Send 'c' or 'C' via Serial Monitor to start calibration
- Keep the device level and still during the 5-second calibration process
- Calibration offsets will be applied to all subsequent readings

### Serial Output Format
```
Accel: X:0.002 Y:-0.001 Z:0.998g | Angles: Roll:0.1° Pitch:-0.1° | Mag:0.998g | Temp:25.3°C | Orient:Face Up [LEVEL]
```

## Hardware Connections

| ESP32 Pin | ADXL355 Pin | Function |
|-----------|-------------|----------|
| GPIO 5    | CS          | Chip Select |
| GPIO 18   | SCLK        | SPI Clock |
| GPIO 19   | MISO        | SPI Data Out |
| GPIO 23   | MOSI        | SPI Data In |
| GPIO 17   | VDD         | Power Enable |
| 3.3V      | VDD         | Power Supply |
| GND       | GND         | Ground |

## Customization

### Modifying Sensor Range
```cpp
// In setup() or during runtime
sensor.setRange(RANGE_4G);  // Options: RANGE_2G, RANGE_4G, RANGE_8G
```

### Adjusting Filter Strength
```cpp
// Lower values = more filtering (smoother but slower response)
angleCalc.setFilterStrength(0.05);  // Range: 0.0 to 1.0
```

### Changing Motion Detection Thresholds
```cpp
// Customize thresholds in angle_calculator.cpp
bool isMotionDetected(float magnitude, float threshold_high = 1.3, float threshold_low = 0.7);
```

## Benefits of Modular Design

1. **Maintainability**: Each module has a single responsibility
2. **Reusability**: Classes can be easily reused in other projects
3. **Testability**: Individual modules can be tested independently
4. **Scalability**: Easy to add new features without affecting existing code
5. **Readability**: Clear separation of concerns makes code easier to understand

## Development Notes

- All sensor-specific constants are centralized in `config.h`
- Error handling is implemented throughout the stack
- The code maintains backward compatibility with the original functionality
- Memory usage is optimized for ESP32 constraints
- Real-time performance is maintained through efficient algorithms

This modular approach transforms the original monolithic code into a professional, maintainable, and extensible accelerometer interface suitable for production use.
