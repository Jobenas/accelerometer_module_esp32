#!/usr/bin/env python3
"""
ADXL355 Modbus Test Script

This script demonstrates how to read data from the ADXL355 Modbus slave.
Requires: pip install pymodbus

Usage:
  python modbus_test.py [port] [baudrate] [slave_id]

Examples:
  python modbus_test.py COM3 9600 1
  python modbus_test.py /dev/ttyUSB0 9600 1
"""

import sys
import struct
import time
from pymodbus.client.sync import ModbusSerialClient
from pymodbus.constants import Endian
from pymodbus.payload import BinaryPayloadDecoder

def parse_float_registers(registers, start_index):
    """Convert two 16-bit registers to IEEE 754 float"""
    if len(registers) < start_index + 2:
        return 0.0
    
    # Modbus sends high register first, then low register
    high_reg = registers[start_index + 1]
    low_reg = registers[start_index]
    
    # Pack as big-endian 32-bit integer, then unpack as float
    combined = (high_reg << 16) | low_reg
    bytes_data = struct.pack('>I', combined)
    return struct.unpack('>f', bytes_data)[0]

def parse_uint32_registers(registers, start_index):
    """Convert two 16-bit registers to 32-bit unsigned integer"""
    if len(registers) < start_index + 2:
        return 0
    
    high_reg = registers[start_index + 1]
    low_reg = registers[start_index]
    return (high_reg << 16) | low_reg

def decode_status_flags(flags):
    """Decode status flags into human readable format"""
    status_names = [
        "DATA_READY", "CALIBRATED", "MOTION_DETECTED", "LEVEL",
        "TILT", "FACE_UP", "FACE_DOWN", "SENSOR_ERROR"
    ]
    
    active_flags = []
    for i, name in enumerate(status_names):
        if flags & (1 << i):
            active_flags.append(name)
    
    return active_flags if active_flags else ["NONE"]

def main():
    # Parse command line arguments
    port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 9600
    slave_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    
    print(f"ADXL355 Modbus Test")
    print(f"Port: {port}, Baudrate: {baudrate}, Slave ID: {slave_id}")
    print("-" * 50)
    
    # Create Modbus client
    client = ModbusSerialClient(
        method='rtu',
        port=port,
        baudrate=baudrate,
        bytesize=8,
        parity='N',
        stopbits=1,
        timeout=1
    )
    
    try:
        # Connect to device
        if not client.connect():
            print(f"Failed to connect to {port}")
            return
        
        print("Connected successfully!")
        print("\nReading sensor data...\n")
        
        # Continuous reading loop
        while True:
            try:
                # Read all sensor registers (40001-40020, which is address 0-19)
                response = client.read_holding_registers(0, 20, unit=slave_id)
                
                if response.isError():
                    print(f"Modbus error: {response}")
                    time.sleep(1)
                    continue
                
                registers = response.registers
                
                # Parse sensor data
                x_accel = parse_float_registers(registers, 0)
                y_accel = parse_float_registers(registers, 2)
                z_accel = parse_float_registers(registers, 4)
                roll = parse_float_registers(registers, 6)
                pitch = parse_float_registers(registers, 8)
                magnitude = parse_float_registers(registers, 10)
                temperature = parse_float_registers(registers, 12)
                status_flags = registers[14]
                timestamp = parse_uint32_registers(registers, 15)
                calibration_status = registers[17]
                device_status = registers[18]
                sample_rate = registers[19]
                
                # Display data
                print(f"\r", end="")  # Clear line
                print(f"Accel: X:{x_accel:+7.3f} Y:{y_accel:+7.3f} Z:{z_accel:+7.3f}g | "
                      f"Angles: Roll:{roll:+6.1f}° Pitch:{pitch:+6.1f}° | "
                      f"Mag:{magnitude:6.3f}g | Temp:{temperature:5.1f}°C", end="")
                
                # Show status occasionally
                if int(time.time()) % 5 == 0:
                    flags = decode_status_flags(status_flags)
                    print(f"\nStatus: {', '.join(flags)}")
                    print(f"Timestamp: {timestamp}ms, Sample Rate: {sample_rate}Hz")
                    print(f"Calibration: {calibration_status}, Device: {device_status}")
                
                time.sleep(0.1)  # 10 Hz display rate
                
            except KeyboardInterrupt:
                print("\n\nStopping...")
                break
            except Exception as e:
                print(f"\nError reading data: {e}")
                time.sleep(1)
    
    finally:
        client.close()
        print("Disconnected.")

if __name__ == "__main__":
    main()
