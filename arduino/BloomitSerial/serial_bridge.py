#!/usr/bin/env python3
"""
Serial Bridge for Bloomit Arduino Integration

Reads JSON sensor data from Arduino via USB/Serial and sends it to the Bloomit API.
Format: {"token":"...","cmd":"sense","sensorType":1,"value":45.5,"text":"optional"}
Log format: {"token":"...","cmd":"log","type":"info","message":"text"}

Note: bloomitweb has a built-in serial proxy, so this script is optional.
Use this only if you need a custom bridge or testing.

Usage:
    python3 serial_bridge.py

Requirements:
    pip3 install pyserial requests
"""

import serial
import serial.tools.list_ports
import json
import requests
import sys
import time

# =============================================================================
# Configuration
# =============================================================================

SERIAL_BAUD = 9600
API_BASE_URL = 'https://device.api.bloomit.app'

# =============================================================================
# Functions
# =============================================================================

def find_arduino_port():
    """Auto-detect Arduino serial port"""
    ports = serial.tools.list_ports.comports()
    
    for port in ports:
        # Common Arduino identifiers
        if 'Arduino' in port.description or \
           'CH340' in port.description or \
           'USB' in port.description or \
           'ttyACM' in port.device or \
           'ttyUSB' in port.device:
            return port.device
    
    return None

def send_sensor_to_api(device_token, sensor_type, value, text=None):
    """Send sensor data to Bloomit API"""
    url = f"{API_BASE_URL}/sense"
    headers = {
        'Content-Type': 'application/json',
        'Authorization': f'Bearer {device_token}'
    }
    # Convert sensorType number to string name for API
    sensor_names = {1: 'temperature', 2: 'humidity'}
    data = {
        'sensorType': sensor_names.get(sensor_type, f'unknown_{sensor_type}'),
        'value': str(value)
    }
    
    # Add optional text field if provided
    if text:
        data['text'] = text
    
    try:
        response = requests.post(url, json=data, headers=headers, timeout=10)
        if response.status_code in [200, 201]:
            return True, response.json()
        else:
            return False, f"HTTP {response.status_code}: {response.text}"
    except Exception as e:
        return False, str(e)

def send_log_to_api(device_token, log_type, message):
    """Send log message to Bloomit API"""
    url = f"{API_BASE_URL}/device/log"
    headers = {
        'Content-Type': 'application/json',
        'Authorization': f'Bearer {device_token}'
    }
    data = {
        'type': log_type,  # "info", "warning", "error", "debug"
        'message': message
    }
    
    try:
        response = requests.post(url, json=data, headers=headers, timeout=10)
        if response.status_code in [200, 201]:
            return True, response.json()
        else:
            return False, f"HTTP {response.status_code}: {response.text}"
    except Exception as e:
        return False, str(e)

def process_message(message):
    """Process JSON message from Arduino"""
    try:
        data = json.loads(message)
        cmd = data.get('cmd', '')
        device_token = data.get('token', '')
        
        if not device_token:
            print("[WARN] Message without token, skipping")
            return
        
        # Handle sensor data: {"token":"...","cmd":"sense","sensorType":1,"value":45.5,"text":"optional"}
        if cmd == 'sense':
            if 'sensorType' in data and 'value' in data:
                sensor_type = data.get('sensorType', 0)  # 1=temperature, 2=humidity
                value = data.get('value', 0)
                text = data.get('text', None)
                
                sensor_names = {1: 'Temperature', 2: 'Humidity'}
                sensor_name = sensor_names.get(sensor_type, f'Type {sensor_type}')
                
                if text:
                    print(f"[SENSOR] {sensor_name}: {value} ({text})")
                else:
                    print(f"[SENSOR] {sensor_name}: {value}")
                
                success, result = send_sensor_to_api(device_token, sensor_type, value, text)
                
                if success:
                    print(f"[OK] Sensor data sent to Bloomit API")
                else:
                    print(f"[ERROR] {result}")
            else:
                print("[ERROR] Invalid sense command format")
        
        # Handle log: {"token":"...","cmd":"log","type":"info","message":"text"}
        elif cmd == 'log':
            log_type = data.get('type', 'info')
            log_message = data.get('message', '')
            
            if log_message:
                print(f"[LOG] {log_type.upper()}: {log_message}")
                
                success, result = send_log_to_api(device_token, log_type, log_message)
                
                if success:
                    print(f"[OK] Log sent to Bloomit API")
                else:
                    print(f"[ERROR] {result}")
            else:
                print("[ERROR] Log message is empty")
        
        # Unknown command
        else:
            print(f"[WARN] Unknown command: {cmd}")
        
    except json.JSONDecodeError:
        # Not JSON, might be debug message
        if message.strip() and not message.startswith('['):
            print(f"[DEBUG] {message}")
    except Exception as e:
        print(f"[ERROR] Processing message: {e}")

def main():
    print("=" * 50)
    print("  Bloomit Serial Bridge")
    print("=" * 50)
    print()
    
    # Find Arduino port
    port = find_arduino_port()
    
    if not port:
        print("[ERROR] No Arduino found!")
        print()
        print("Available ports:")
        for p in serial.tools.list_ports.comports():
            print(f"  - {p.device}: {p.description}")
        print()
        print("Connect your Arduino and try again.")
        sys.exit(1)
    
    print(f"[OK] Found Arduino on {port}")
    print(f"[OK] Connecting at {SERIAL_BAUD} baud...")
    print()
    
    try:
        ser = serial.Serial(port, SERIAL_BAUD, timeout=1)
        time.sleep(2)  # Wait for Arduino reset
        print("[OK] Connected! Waiting for sensor data...")
        print("     Press Ctrl+C to exit")
        print()
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open port: {e}")
        print()
        print("Try: sudo chmod 666 " + port)
        sys.exit(1)
    
    try:
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    process_message(line)
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        print("\n\n[OK] Shutting down...")
    finally:
        ser.close()
        print("[OK] Serial port closed")

if __name__ == '__main__':
    main()
