@echo off
echo ADXL355 Modbus Test Setup
echo ========================

echo.
echo Installing required Python packages...
pip install pymodbus

echo.
echo Testing Modbus connectivity...
echo.
echo Make sure your ESP32 is:
echo 1. Connected to Serial2 (GPIO 16/17)
echo 2. Running in Modbus mode (send 'm' at startup)
echo 3. Connected to your Modbus master device
echo.

set /p port="Enter COM port (e.g., COM3): "
if "%port%"=="" set port=COM3

echo.
echo Running simple connectivity test...
python simple_test.py %port% 9600 1

echo.
set /p continuous="Run continuous monitoring? (y/n): "
if /i "%continuous%"=="y" (
    echo.
    echo Starting continuous monitoring...
    echo Press Ctrl+C to stop
    python test_modbus.py %port% 9600 1
)

echo.
echo Test complete!
pause
