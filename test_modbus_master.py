#!/usr/bin/env python3
"""
Modbus Master Test Script for ADXL355 ESP32 Project

This script acts as a Modbus RTU master to communicate with the ESP32 slave device.
It reads both holding and input registers to verify the Modbus interface is working.

Requirements:
pip install pymodbus pyserial

Usage:
1. Upload the firmware to ESP32
2. Connect ESP32 Serial2 (pins 16/17) to USB-RS485 adapter
3. Connect DE/RE pin 4 to the direction control of RS485 adapter
4. Update COM_PORT below to match your USB-RS485 adapter
5. Run this script

Register Map:
- Holding registers (0-4): Device info, config
- Input registers (0-23): Analytics data, system status
"""

import time
import struct
from pymodbus.client import ModbusSerialClient
from pymodbus.exceptions import ModbusException

# Configuration
COM_PORT = 'COM3'  # Change this to match your USB-RS485 adapter
BAUDRATE = 9600
SLAVE_ID = 1
TIMEOUT = 1.0

# Register definitions (must match the ESP32 code)
# Holding registers
REG_DEVICE_ID = 0
REG_FIRMWARE_VERSION = 1
REG_SAMPLE_RATE = 2
REG_WINDOW_COUNT_LOW = 3
REG_WINDOW_COUNT_HIGH = 4

# Input registers (read-only) - starting at address 5 (after holding registers)
REG_CURRENT_AVG_X = 5
REG_CURRENT_AVG_Y = 6
REG_CURRENT_AVG_Z = 7
REG_CURRENT_MAX_X = 8
REG_CURRENT_MAX_Y = 9
REG_CURRENT_MAX_Z = 10
REG_CURRENT_MIN_X = 11
REG_CURRENT_MIN_Y = 12
REG_CURRENT_MIN_Z = 13

REG_RUNNING_AVG_X = 14
REG_RUNNING_AVG_Y = 15
REG_RUNNING_AVG_Z = 16
REG_GLOBAL_MAX_X = 17
REG_GLOBAL_MAX_Y = 18
REG_GLOBAL_MAX_Z = 19
REG_GLOBAL_MIN_X = 20
REG_GLOBAL_MIN_Y = 21
REG_GLOBAL_MIN_Z = 22

REG_TASK_STATUS = 23
REG_SAMPLING_ERRORS = 24
REG_PROCESSING_ERRORS = 25
REG_ANALYTICS_ERRORS = 26
REG_MISSED_SAMPLES = 27
REG_LAST_UPDATE_TIME = 28

SCALE_FACTOR = 1000.0

def scaled_int_to_float(value):
    """Convert scaled integer back to float"""
    # Handle signed 16-bit integer
    if value > 32767:
        value = value - 65536
    return value / SCALE_FACTOR

def read_holding_registers(client):
    """Read and display holding registers"""
    print("\n=== HOLDING REGISTERS (Configuration) ===")
    
    try:
        result = client.read_holding_registers(0, 5, slave=SLAVE_ID)
        if result.isError():
            print(f"Error reading holding registers: {result}")
            return False
        
        registers = result.registers
        
        print(f"Device ID: 0x{registers[REG_DEVICE_ID]:04X}")
        print(f"Firmware Version: {registers[REG_FIRMWARE_VERSION] / 100:.2f}")
        print(f"Sample Rate: {registers[REG_SAMPLE_RATE]} Hz")
        
        # Combine 32-bit window count
        window_count = (registers[REG_WINDOW_COUNT_HIGH] << 16) | registers[REG_WINDOW_COUNT_LOW]
        print(f"Window Count: {window_count}")
        
        return True
        
    except Exception as e:
        print(f"Exception reading holding registers: {e}")
        return False

def read_input_registers(client):
    """Read and display input registers (analytics data)"""
    print("\n=== INPUT REGISTERS (Analytics Data) ===")
    
    try:
        # Read all registers as holding registers since the library combines them
        result = client.read_holding_registers(0, 29, slave=SLAVE_ID)  # 5 holding + 24 input
        if result.isError():
            print(f"Error reading registers: {result}")
            return False
        
        registers = result.registers
        
        # Current window statistics
        print("\nCurrent Window Statistics:")
        print(f"  Average: X={scaled_int_to_float(registers[REG_CURRENT_AVG_X]):.3f}, "
              f"Y={scaled_int_to_float(registers[REG_CURRENT_AVG_Y]):.3f}, "
              f"Z={scaled_int_to_float(registers[REG_CURRENT_AVG_Z]):.3f}")
        
        print(f"  Maximum: X={scaled_int_to_float(registers[REG_CURRENT_MAX_X]):.3f}, "
              f"Y={scaled_int_to_float(registers[REG_CURRENT_MAX_Y]):.3f}, "
              f"Z={scaled_int_to_float(registers[REG_CURRENT_MAX_Z]):.3f}")
        
        print(f"  Minimum: X={scaled_int_to_float(registers[REG_CURRENT_MIN_X]):.3f}, "
              f"Y={scaled_int_to_float(registers[REG_CURRENT_MIN_Y]):.3f}, "
              f"Z={scaled_int_to_float(registers[REG_CURRENT_MIN_Z]):.3f}")
        
        # Running statistics
        print("\nRunning Statistics:")
        print(f"  Average: X={scaled_int_to_float(registers[REG_RUNNING_AVG_X]):.3f}, "
              f"Y={scaled_int_to_float(registers[REG_RUNNING_AVG_Y]):.3f}, "
              f"Z={scaled_int_to_float(registers[REG_RUNNING_AVG_Z]):.3f}")
        
        print(f"  Global Max: X={scaled_int_to_float(registers[REG_GLOBAL_MAX_X]):.3f}, "
              f"Y={scaled_int_to_float(registers[REG_GLOBAL_MAX_Y]):.3f}, "
              f"Z={scaled_int_to_float(registers[REG_GLOBAL_MAX_Z]):.3f}")
        
        print(f"  Global Min: X={scaled_int_to_float(registers[REG_GLOBAL_MIN_X]):.3f}, "
              f"Y={scaled_int_to_float(registers[REG_GLOBAL_MIN_Y]):.3f}, "
              f"Z={scaled_int_to_float(registers[REG_GLOBAL_MIN_Z]):.3f}")
        
        # System status
        print("\nSystem Status:")
        task_status = registers[REG_TASK_STATUS]
        print(f"  Task Status: 0x{task_status:04X}")
        print(f"    Sampling Task: {'Running' if (task_status & 0x0001) else 'Stopped'}")
        print(f"    Processing Task: {'Running' if (task_status & 0x0002) else 'Stopped'}")
        print(f"    Analytics Task: {'Running' if (task_status & 0x0004) else 'Stopped'}")
        print(f"    Modbus Task: {'Running' if (task_status & 0x0008) else 'Stopped'}")
        
        print(f"  Error Counts:")
        print(f"    Sampling: {registers[REG_SAMPLING_ERRORS]}")
        print(f"    Processing: {registers[REG_PROCESSING_ERRORS]}")
        print(f"    Analytics: {registers[REG_ANALYTICS_ERRORS]}")
        print(f"    Missed Samples: {registers[REG_MISSED_SAMPLES]}")
        
        print(f"  Last Update: {registers[REG_LAST_UPDATE_TIME]} ms ago")
        
        return True
        
    except Exception as e:
        print(f"Exception reading input registers: {e}")
        return False

def main():
    print("ADXL355 ESP32 Modbus Master Test")
    print("=================================")
    print(f"Connecting to {COM_PORT} at {BAUDRATE} baud...")
    print(f"Slave ID: {SLAVE_ID}")
    
    # Create Modbus client
    client = ModbusSerialClient(
        port=COM_PORT,
        baudrate=BAUDRATE,
        timeout=TIMEOUT,
        parity='N',
        stopbits=1,
        bytesize=8
    )
    
    try:
        # Connect to the device
        if not client.connect():
            print("Failed to connect to Modbus device!")
            print("Check:")
            print("1. COM port is correct")
            print("2. ESP32 is connected and running")
            print("3. RS485 adapter is connected properly")
            print("4. Baud rate matches (9600)")
            return
        
        print("Connected successfully!")
        
        # Test communication continuously
        test_count = 0
        while True:
            test_count += 1
            print(f"\n{'='*60}")
            print(f"Test #{test_count} - {time.strftime('%H:%M:%S')}")
            print(f"{'='*60}")
            
            # Read holding registers
            if read_holding_registers(client):
                print("✓ Holding registers read successfully")
            else:
                print("✗ Failed to read holding registers")
            
            # Read input registers
            if read_input_registers(client):
                print("✓ Input registers read successfully")
            else:
                print("✗ Failed to read input registers")
            
            # Wait before next test
            print(f"\nWaiting 5 seconds before next test...")
            time.sleep(5)
            
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    except Exception as e:
        print(f"Unexpected error: {e}")
    finally:
        client.close()
        print("Modbus connection closed")

if __name__ == "__main__":
    main()
