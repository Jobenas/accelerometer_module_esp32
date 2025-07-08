#!/usr/bin/env python3
"""
Simple ADXL355 Modbus connectivity test

This script performs basic connectivity tests:
1. Read device status
2. Read acceleration data
3. Read temperature

Usage: python simple_test.py [port] [baudrate] [slave_id]
"""

import sys
import struct
from pymodbus.client.sync import ModbusSerialClient

def float_from_registers(reg_low, reg_high):
    """Convert two Modbus registers to IEEE 754 float"""
    combined = (reg_high << 16) | reg_low
    bytes_data = struct.pack('>I', combined)
    return struct.unpack('>f', bytes_data)[0]

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 9600
    slave_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    
    print(f"ADXL355 Simple Modbus Test")
    print(f"Port: {port}, Baudrate: {baudrate}, Slave ID: {slave_id}")
    
    client = ModbusSerialClient(
        method='rtu',
        port=port,
        baudrate=baudrate,
        timeout=2
    )
    
    if not client.connect():
        print("❌ Failed to connect")
        return
    
    print("✅ Connected!")
    
    try:
        # Test 1: Read status flags (register 40015 = address 14)
        print("\n1. Reading status flags...")
        response = client.read_holding_registers(14, 1, unit=slave_id)
        if response.isError():
            print(f"❌ Error: {response}")
        else:
            flags = response.registers[0]
            print(f"✅ Status flags: 0x{flags:04X}")
        
        # Test 2: Read X acceleration (registers 40001-40002 = addresses 0-1)
        print("\n2. Reading X acceleration...")
        response = client.read_holding_registers(0, 2, unit=slave_id)
        if response.isError():
            print(f"❌ Error: {response}")
        else:
            x_accel = float_from_registers(response.registers[0], response.registers[1])
            print(f"✅ X acceleration: {x_accel:.3f}g")
        
        # Test 3: Read temperature (registers 40013-40014 = addresses 12-13)
        print("\n3. Reading temperature...")
        response = client.read_holding_registers(12, 2, unit=slave_id)
        if response.isError():
            print(f"❌ Error: {response}")
        else:
            temp = float_from_registers(response.registers[0], response.registers[1])
            print(f"✅ Temperature: {temp:.1f}°C")
        
        # Test 4: Read all acceleration data
        print("\n4. Reading all acceleration data...")
        response = client.read_holding_registers(0, 6, unit=slave_id)
        if response.isError():
            print(f"❌ Error: {response}")
        else:
            regs = response.registers
            x = float_from_registers(regs[0], regs[1])
            y = float_from_registers(regs[2], regs[3])
            z = float_from_registers(regs[4], regs[5])
            print(f"✅ Acceleration: X={x:.3f}, Y={y:.3f}, Z={z:.3f}g")
        
        print("\n🎉 All tests completed successfully!")
        
    except Exception as e:
        print(f"❌ Exception: {e}")
    
    finally:
        client.close()
        print("Disconnected.")

if __name__ == "__main__":
    main()
