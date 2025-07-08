# PlatformIO Build Environments Guide

This document explains the proper PlatformIO project structure and how to use the different build environments for testing and development.

## Why Use PlatformIO Structure (Not .ino files)

This project uses PlatformIO with proper `.cpp` files instead of Arduino `.ino` files for several important reasons:

### ✅ Benefits of PlatformIO Structure:
- **Modular Architecture**: Clean separation of code into header (.h) and implementation (.cpp) files
- **Multiple Build Environments**: Different configurations for production, testing, and debugging
- **Better Code Organization**: Proper include paths, dependencies, and library management
- **Advanced Build System**: Conditional compilation, build flags, and environment-specific settings
- **Professional Development**: Industry-standard project structure and tooling
- **Easy CI/CD Integration**: Automated builds and testing
- **Library Management**: Precise dependency control and version management

### ❌ Problems with .ino Files:
- **Global Scope Pollution**: All code shares global namespace
- **Poor Modularity**: Difficult to separate concerns and create reusable modules
- **Limited Build Options**: No easy way to create different build configurations
- **Dependency Issues**: Hard to manage complex library dependencies
- **Poor Version Control**: Large monolithic files are hard to track changes

## Build Environments

This project provides three build environments in `platformio.ini`:

### 1. Production Environment (`esp32dev`)
```bash
pio run -e esp32dev
pio run -e esp32dev --target upload
```
- **Purpose**: Full application with all features enabled
- **Features**: ADXL355 sensor, FreeRTOS tasks, analytics, Modbus RTU interface
- **Use Case**: Deploy to production hardware

### 2. Serial Monitor Test (`esp32dev_serial_test`)
```bash
pio run -e esp32dev_serial_test
pio run -e esp32dev_serial_test --target upload
```
- **Purpose**: Raw serial monitoring for debugging Modbus communication
- **Features**: Monitors Serial2 and prints received bytes in hex format
- **Use Case**: Debug if Modbus requests reach the ESP32
- **Build Flag**: `-DSERIAL_MONITOR_TEST=1`

### 3. Simple Modbus Test (`esp32dev_modbus_test`)
```bash
pio run -e esp32dev_modbus_test
pio run -e esp32dev_modbus_test --target upload
```
- **Purpose**: Minimal Modbus RTU slave with test data
- **Features**: Simple register array (values 1-16) for basic Modbus testing
- **Use Case**: Verify Modbus library and hardware functionality
- **Build Flag**: `-DMODBUS_TEST_MODE=1`

## How the Build System Works

### Conditional Compilation in main.cpp
The `main.cpp` file uses preprocessor directives to compile different code based on build flags:

```cpp
#ifdef SERIAL_MONITOR_TEST
  #define ACTIVE_TEST_MODE 1
#elif defined(MODBUS_TEST_MODE)
  #define ACTIVE_TEST_MODE 2
#else
  #define ACTIVE_TEST_MODE 0  // Production mode
#endif

#if ACTIVE_TEST_MODE == 1
  // Serial monitor test code
#elif ACTIVE_TEST_MODE == 2
  // Simple Modbus test code
#else
  // Full production application
#endif
```

### Environment-Specific Libraries
- **Production & Modbus Test**: Include Modbus library
- **Serial Test**: Only basic ESP32 libraries (smaller binary)

## Project Structure

```
├── platformio.ini          # Build environments and configurations
├── include/                 # Header files (.h)
│   ├── config.h            # Global configuration
│   ├── adxl355.h           # Sensor interface
│   ├── data_buffer.h       # Data buffer management
│   ├── analytics.h         # Analytics engine
│   ├── modbus_interface.h  # Modbus RTU interface
│   └── task_manager.h      # FreeRTOS task management
├── src/                     # Implementation files (.cpp)
│   ├── main.cpp            # Main application with test modes
│   ├── adxl355.cpp         # Sensor implementation
│   ├── data_buffer.cpp     # Buffer implementation
│   ├── analytics.cpp       # Analytics implementation
│   ├── modbus_interface.cpp # Modbus implementation
│   └── task_manager.cpp    # Task implementation
└── lib/                     # Custom libraries (if any)
```

## Testing Workflow

### Step 1: Test Serial Communication
```bash
pio run -e esp32dev_serial_test --target upload
pio device monitor -e esp32dev_serial_test
```
Send Modbus requests from PC and verify ESP32 receives data.

### Step 2: Test Basic Modbus
```bash
pio run -e esp32dev_modbus_test --target upload
pio device monitor -e esp32dev_modbus_test
```
Use a Modbus master to read registers 0-15 and verify responses.

### Step 3: Deploy Production
```bash
pio run -e esp32dev --target upload
pio device monitor -e esp32dev
```
Full application with sensor data and analytics.

## Configuration Management

### Build Flags
- Set in `platformio.ini` under `build_flags`
- Available to all source files via preprocessor
- Example: `-DSERIAL_MONITOR_TEST=1`

### Config Header (`include/config.h`)
- Runtime configuration options
- Feature enable/disable flags
- Pin definitions and constants

### Environment Variables
- PlatformIO can use environment variables
- Useful for sensitive data or CI/CD

## Debugging Tips

1. **Use the Right Environment**: Always select the appropriate build environment for your testing needs
2. **Monitor Output**: Use `pio device monitor` with the same environment used for upload
3. **Check Build Logs**: Compilation warnings can indicate potential issues
4. **Incremental Testing**: Start with simple tests before moving to complex features

## Library Dependencies

Libraries are automatically managed by PlatformIO based on the `lib_deps` section in `platformio.ini`:

- **SPI@2.0.0**: For sensor communication
- **Modbus-Master-Slave-for-Arduino**: For Modbus RTU functionality

## Best Practices

1. **Keep main.cpp Lean**: Business logic should be in separate modules
2. **Use Forward Declarations**: Minimize include dependencies in headers
3. **Test Incrementally**: Use the different environments to isolate issues
4. **Document Changes**: Update this guide when adding new environments or features
5. **Version Control**: Track all changes in git with meaningful commit messages

This structure provides a professional, maintainable, and scalable foundation for ESP32/Arduino development.
