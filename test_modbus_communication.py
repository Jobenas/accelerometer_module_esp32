"""
Simple Modbus RTU Test Script for ESP32 Accelerometer Module

Requirements:
pip install pymodbus

Usage:
python test_modbus_communication.py COM3

Replace COM3 with your actual COM port where the MAX485 is connected.
"""

import sys
import time
from pymodbus.client import ModbusSerialClient

def test_device_id(client):
    """Test reading device ID (should return 4660/0x1234)"""
    print("Testing Device ID (Holding Register 0)...")
    try:
        result = client.read_holding_registers(address=0, count=1, slave=1)
        if result.isError():
            print(f"❌ Error reading Device ID: {result}")
            return False
        else:
            device_id = result.registers[0]
            print(f"✅ Device ID: {device_id} (0x{device_id:04X})")
            if device_id == 4660:  # 0x1234
                print("✅ Device ID matches expected value!")
                return True
            else:
                print(f"⚠️  Device ID doesn't match expected 4660")
                return False
    except Exception as e:
        print(f"❌ Exception reading Device ID: {e}")
        return False

def test_firmware_version(client):
    """Test reading firmware version (should return 100)"""
    print("\nTesting Firmware Version (Holding Register 1)...")
    try:
        result = client.read_holding_registers(address=1, count=1, slave=1)
        if result.isError():
            print(f"❌ Error reading Firmware Version: {result}")
            return False
        else:
            version = result.registers[0]
            print(f"✅ Firmware Version: {version} (v{version/100:.2f})")
            return True
    except Exception as e:
        print(f"❌ Exception reading Firmware Version: {e}")
        return False

def test_accelerometer_data(client):
    """Test reading accelerometer data (Input Registers 0-2)"""
    print("\nTesting Accelerometer Data (Input Registers 0-2)...")
    try:
        result = client.read_input_registers(address=0, count=3, slave=1)
        if result.isError():
            print(f"❌ Error reading Accelerometer Data: {result}")
            return False
        else:
            # Convert from signed 16-bit to actual values
            x_raw = result.registers[0]
            y_raw = result.registers[1] 
            z_raw = result.registers[2]
            
            # Convert to signed values
            def to_signed(val):
                return val if val < 32768 else val - 65536
            
            x_signed = to_signed(x_raw)
            y_signed = to_signed(y_raw)
            z_signed = to_signed(z_raw)
            
            # Convert to g-force (scale factor 1000)
            x_g = x_signed / 1000.0
            y_g = y_signed / 1000.0
            z_g = z_signed / 1000.0
            
            print(f"✅ Raw values: X={x_raw}, Y={y_raw}, Z={z_raw}")
            print(f"✅ Signed values: X={x_signed}, Y={y_signed}, Z={z_signed}")
            print(f"✅ G-force values: X={x_g:.3f}g, Y={y_g:.3f}g, Z={z_g:.3f}g")
            
            # Check if values are reasonable (accelerometer should show ~1g in one axis when static)
            magnitude = (x_g**2 + y_g**2 + z_g**2)**0.5
            print(f"✅ Magnitude: {magnitude:.3f}g")
            
            if 0.5 < magnitude < 2.0:  # Reasonable range for static accelerometer
                print("✅ Accelerometer data looks reasonable!")
                return True
            else:
                print("⚠️  Accelerometer data may be unrealistic (check sensor)")
                return True  # Still counts as communication success
                
    except Exception as e:
        print(f"❌ Exception reading Accelerometer Data: {e}")
        return False

def test_system_status(client):
    """Test reading system status (Input Register 18)"""
    print("\nTesting System Status (Input Register 18)...")
    try:
        result = client.read_input_registers(address=18, count=1, slave=1)
        if result.isError():
            print(f"❌ Error reading System Status: {result}")
            return False
        else:
            status = result.registers[0]
            print(f"✅ System Status: {status} (0b{status:016b})")
            
            # Decode status bits
            print("Status flags:")
            print(f"  Bit 0 (Sampling task): {'Running' if status & 0x01 else 'Stopped'}")
            print(f"  Bit 1 (Processing task): {'Running' if status & 0x02 else 'Stopped'}")
            print(f"  Bit 2 (Analytics task): {'Running' if status & 0x04 else 'Stopped'}")
            print(f"  Bit 3 (Modbus task): {'Running' if status & 0x08 else 'Stopped'}")
            
            return True
    except Exception as e:
        print(f"❌ Exception reading System Status: {e}")
        return False

def main():
    if len(sys.argv) != 2:
        print("Usage: python test_modbus_communication.py <COM_PORT>")
        print("Example: python test_modbus_communication.py COM3")
        sys.exit(1)
    
    com_port = sys.argv[1]
    
    print("="*60)
    print("ESP32 Accelerometer Module - Modbus Communication Test")
    print("="*60)
    print(f"Port: {com_port}")
    print(f"Baudrate: 9600")
    print(f"Slave ID: 1")
    print("-"*60)
    
    # Create Modbus client
    client = ModbusSerialClient(
        port=com_port,
        baudrate=9600,
        bytesize=8,
        parity='N',
        stopbits=1,
        timeout=2
    )
    
    if not client.connect():
        print(f"❌ Failed to connect to {com_port}")
        print("Check:")
        print("  - COM port is correct")
        print("  - MAX485 wiring is correct")
        print("  - ESP32 is powered and running")
        sys.exit(1)
    
    print(f"✅ Connected to {com_port}")
    
    # Run tests
    tests_passed = 0
    total_tests = 4
    
    if test_device_id(client):
        tests_passed += 1
    
    if test_firmware_version(client):
        tests_passed += 1
        
    if test_accelerometer_data(client):
        tests_passed += 1
        
    if test_system_status(client):
        tests_passed += 1
    
    # Summary
    print("\n" + "="*60)
    print(f"Test Results: {tests_passed}/{total_tests} tests passed")
    
    if tests_passed == total_tests:
        print("🎉 All tests passed! Modbus communication is working perfectly!")
    elif tests_passed > 0:
        print("⚠️  Some tests passed - communication is working but there may be issues")
    else:
        print("❌ No tests passed - check your wiring and configuration")
    
    print("="*60)
    
    client.close()

if __name__ == "__main__":
    main()
