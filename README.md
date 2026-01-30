# Bloomit IoT Device Integration Examples

<p align="center">
  <img src="https://app.bloomit.com.ar/img/logo-full.png" alt="Bloomit Logo" width="300">
</p>

## 🌱 What is Bloomit?

**Bloomit** is an IoT plant monitoring platform that helps you keep your plants healthy by collecting and analyzing sensor data in real-time. Connect your custom ESP32, Arduino, or any HTTP-capable device to the Bloomit cloud and get actionable insights about your plants.

### Key Features

| Feature | Description |
|---------|-------------|
| 📊 **Smart Monitoring** | Track soil humidity, temperature, air humidity, pH, TDS, and more with connected sensors |
| 🌿 **Plant Insights** | Get personalized recommendations based on your plant species and sensor readings |
| 🔔 **Real-time Alerts** | Receive notifications when your plants need attention or conditions are suboptimal |
| 📱 **Multi-platform** | Monitor your plants from the mobile app (iOS/Android) or web dashboard |
| 🔌 **Multi-device Support** | Connect and manage multiple Bloomit devices from a single account |

### Bloomit Ecosystem

| Platform | URL | Description |
|----------|-----|-------------|
| 🌐 **Website** | [bloomit.com.ar](https://bloomit.com.ar) | Main website and store |
| 📱 **Web App** | [app.bloomit.com.ar](https://app.bloomit.com.ar) | Web dashboard for monitoring |
| 🔧 **Device API** | `device.api.bloomit.app` | API for IoT device integration |
| 👤 **User API** | `user.api.bloomit.app` | API for user authentication |

---

## 📖 About This Repository

This repository contains example implementations for integrating IoT devices with the Bloomit Device API. These examples demonstrate device registration, sensor data submission, and logging functionality.

## Table of Contents

- [What is Bloomit?](#-what-is-bloomit)
- [About This Repository](#-about-this-repository)
- [Overview](#overview)
- [API Reference](#api-reference)
- [Arduino Example](#arduino-example)
- [ESP32 (ESP-IDF) Example](#esp32-esp-idf-example)
- [Getting Started](#getting-started)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [Support & Resources](#support--resources)

## Overview

The Bloomit Device API allows IoT devices to:

- **Register** with a user account using a user token
- **Send sensor data** (temperature, humidity, soil moisture, etc.)
- **Log messages** for debugging and monitoring
- **Retrieve device information** and MQTT subscriptions

### Architecture

```
┌─────────────────┐     HTTPS     ┌─────────────────────────────┐
│   IoT Device    │ ────────────► │   device.api.bloomit.app    │
│                 │ ◄──────────── │      (Device API)           │
└─────────────────┘               └─────────────────────────────┘
```

### Authentication Flow

```
1. User Login (one-time setup)
   ┌──────────────────┐         ┌────────────────────────┐
   │  Bloomit App /   │  POST   │  user.api.bloomit.app  │
   │  Web Client      │ ──────► │  /auth/login           │
   └──────────────────┘         └────────────────────────┘
                                         │
                                         ▼
                                   user_token (JWT)

2. Device Registration (one-time per device)
   ┌──────────────────┐         ┌────────────────────────┐
   │   IoT Device     │  POST   │ device.api.bloomit.app │
   │                  │ ──────► │ /register/public       │
   └──────────────────┘         └────────────────────────┘
         │                               │
         │  { userToken, device_id }     │
         └───────────────────────────────┘
                                         │
                                         ▼
                              device_token (never expires)

3. Device Operations (ongoing)
   ┌──────────────────┐         ┌────────────────────────┐
   │   IoT Device     │  POST   │ device.api.bloomit.app │
   │                  │ ──────► │ /sense, /device/log    │
   └──────────────────┘         └────────────────────────┘
         │                               
         │  Authorization: Bearer device_token
         └───────────────────────────────────────
```

## API Reference

**Base URL:** `https://device.api.bloomit.app`

### Endpoints

| Endpoint | Method | Auth Required | Description |
|----------|--------|---------------|-------------|
| `/register/public` | POST | No | Register device with user token |
| `/sense` | POST | Yes (Bearer) | Submit sensor reading |
| `/device/log` | POST | Yes (Bearer) | Send log message |
| `/api/device` | GET | Yes (Bearer) | Get device information |

### Device Registration

**Endpoint:** `POST /register/public`

**Request:**
```json
{
  "userToken": "eyJhbGciOiJIUzI1NiIs...",
  "device_registration_id": "REG-AABBCCDDEEFF"
}
```

**Response (Success - 201):**
```json
{
  "success": true,
  "message": "Device registered successfully",
  "data": {
    "device_id": 123,
    "user_id": 456,
    "is_active": true
  },
  "device_token": "eyJhbGciOiJIUzI1NiIs...",
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

### Send Sensor Data

**Endpoint:** `POST /sense`

**Headers:**
```
Authorization: Bearer <device_token>
Content-Type: application/json
```

**Request:**
```json
{
  "sensorType": "temperature",
  "value": "25.5"
}
```

**Supported Sensor Types:**
- `temperature` - Temperature in Celsius
- `humidity` - Relative humidity percentage
- `soil_humidity` - Soil moisture percentage
- `ph` - pH level
- `tds` - Total dissolved solids (ppm)

**Response (Success - 201):**
```json
{
  "success": true,
  "message": "Sensor reading added successfully",
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

### Send Log Message

**Endpoint:** `POST /device/log`

**Headers:**
```
Authorization: Bearer <device_token>
Content-Type: application/json
```

**Request:**
```json
{
  "type": "info",
  "message": "Device started successfully"
}
```

**Log Types:**
- `info` - Informational messages
- `warning` - Warning conditions
- `error` - Error conditions
- `debug` - Debug information

**Response (Success - 201):**
```json
{
  "success": true,
  "message": "Log entry created successfully",
  "data": {
    "id": 789,
    "type": "info",
    "message": "Device started successfully",
    "created_at": "2024-01-15T10:30:00.000Z"
  },
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

### Get Device Info

**Endpoint:** `GET /api/device`

**Headers:**
```
Authorization: Bearer <device_token>
```

**Response (Success - 200):**
```json
{
  "success": true,
  "data": {
    "device_id": 123,
    "user_id": 456,
    "is_active": true,
    "last_time_connected": "2024-01-15T10:30:00.000Z"
  },
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

### Device Registration ID Format

The device registration ID is generated from the device's MAC address:

```
REG-{MAC_ADDRESS}
```

Example: `REG-AABBCCDDEEFF`

## Arduino Example

### Location
`arduino/BloomitExample/BloomitExample.ino`

### Requirements

- **Hardware:** Any Arduino-compatible board with WiFi
- **Arduino IDE:** Version 2.0 or later

### Required Libraries

Install via Arduino Library Manager:
- **ArduinoJson** (v7.0.0 or later)

> **Note:** The example uses WiFi.h, HTTPClient.h, and Preferences.h which may need to be adapted for your specific board. See comments in the code.

### Features

- WiFi connection with automatic reconnection
- Device registration with MAC-based registration ID
- Persistent device token storage (token never expires)
- Sensor data submission (temperature, humidity)
- Device logging (info, warning, error, debug)
- Serial command interface for debugging

### Setup Instructions

1. **Open the sketch** in Arduino IDE:
   ```
   arduino/BloomitExample/BloomitExample.ino
   ```

2. **Configure WiFi credentials:**
   ```cpp
   const char* WIFI_SSID = "YOUR_WIFI_SSID";
   const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   ```

3. **Set the user token:**
   ```cpp
   const char* USER_TOKEN = "YOUR_USER_TOKEN_HERE";
   ```
   
   > Get the user token from the Bloomit mobile app when linking a device.

4. **Select the board:**
   - Tools → Board → ESP32 Arduino → ESP32 Dev Module

5. **Upload the sketch**

### Serial Commands

Connect at 115200 baud to use these commands:
- `status` - Display device status
- `register` - Force re-registration
- `info` - Get device info from API
- `sense` - Send sensor readings now
- `log <message>` - Send a log message
- `clear` - Clear stored credentials
- `help` - Show available commands

---

## ESP32 (ESP-IDF) Example

### Location
`esp32/`

### Requirements

- **ESP-IDF:** Version 5.0 or later
- **Target:** ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6

### Setup Instructions

1. **Set up ESP-IDF environment:**
   ```bash
   . $HOME/esp/esp-idf/export.sh
   # or on Windows:
   # %IDF_PATH%\export.bat
   ```

2. **Navigate to the project:**
   ```bash
   cd esp32
   ```

3. **Configure the project:**
   
   Edit `main/main.c` and update:
   ```c
   #define WIFI_SSID       "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"
   #define USER_TOKEN      "YOUR_USER_TOKEN_HERE"
   ```

4. **Set the target chip:**
   ```bash
   idf.py set-target esp32
   ```

5. **Build the project:**
   ```bash
   idf.py build
   ```

6. **Flash and monitor:**
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

### Features

- WiFi station mode with event-driven architecture
- Secure HTTPS communication via `esp_http_client`
- NVS persistent storage for credentials
- cJSON for JSON parsing/serialization
- Device registration, sensor data, and logging
- Modular API functions for easy integration

### Project Structure

```
esp32/
├── CMakeLists.txt          # Project build file
├── sdkconfig.defaults      # Default SDK configuration
└── main/
    ├── CMakeLists.txt      # Component build file
    └── main.c              # Main application
```

## Getting Started

### Step 1: Get a User Token

**Option A: Via Bloomit App**
1. Download the Bloomit mobile app
2. Create an account or log in
3. Navigate to "Add Device"
4. The app will display a user token for device linking

**Option B: Via API**
```bash
curl -X POST https://user.api.bloomit.app/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email": "your@email.com", "password": "your_password"}'
```

The response will contain a `token` field - this is your user token.

### Step 2: Configure Your Device

#### Arduino
1. Open `arduino/BloomitExample/BloomitExample.ino`
2. Set `WIFI_SSID`, `WIFI_PASSWORD`, and `USER_TOKEN`
3. Upload to your ESP32

#### ESP-IDF
1. Edit `esp32/main/main.c`
2. Set `WIFI_SSID`, `WIFI_PASSWORD`, and `USER_TOKEN`
3. Build and flash

### Step 3: Verify Registration

After the device boots:
1. Check serial output for "Device registered successfully!"
2. The device will automatically store its `device_token`
3. The `device_token` **never expires** - it's used for all future API calls
4. Subsequent boots will use the stored token automatically

### Step 4: Monitor Data

- View sensor data in the Bloomit mobile app
- Check device logs in the app's device detail view

## Configuration

### WiFi Settings

| Parameter | Description | Example |
|-----------|-------------|---------|
| `WIFI_SSID` | Your WiFi network name | `"MyHomeWiFi"` |
| `WIFI_PASSWORD` | Your WiFi password | `"secretpassword123"` |

### API Settings

| Parameter | Description | Value |
|-----------|-------------|-------|
| `API_BASE_URL` | Device API endpoint | `https://device.api.bloomit.app` |
| `USER_TOKEN` | Token from user login | Required for registration only |

### Token Types

| Token | How to Get | Expiration | Usage |
|-------|-----------|------------|-------|
| `user_token` | Login at `user.api.bloomit.app` | May expire | Used once for device registration |
| `device_token` | Returned from `/register/public` | **Never expires** | Used for all device API calls |

### Timing Settings (Arduino)

| Interval | Default | Description |
|----------|---------|-------------|
| Sensor readings | 30 seconds | Time between sensor data submissions |
| Heartbeat | 5 minutes | Time between heartbeat log messages |

## Troubleshooting

### Common Issues

#### "Device registration failed"
- Verify `USER_TOKEN` is correct and not expired
- Check WiFi connection
- Ensure API URL is correct

#### "HTTP request failed"
- Check WiFi connectivity
- Verify HTTPS is working (certificate issues)
- Check API endpoint availability

#### "Cannot send sensor data: device not registered"
- Wait for registration to complete
- Check if credentials are stored correctly
- Try clearing credentials and re-registering

### Debug Tips

1. **Enable verbose logging:**
   - Arduino: Use `Serial.println()` statements
   - ESP-IDF: Set log level to DEBUG in menuconfig

2. **Check stored credentials:**
   - Arduino: Use `status` serial command
   - ESP-IDF: Check NVS contents

3. **Verify network connectivity:**
   - Check WiFi signal strength
   - Test API endpoint with curl

### Factory Reset

To clear all stored credentials and force re-registration:

**Arduino:**
```
Serial command: clear
Serial command: register
```

**ESP-IDF:**
```bash
idf.py erase-flash
idf.py flash
```

## License

This project is provided as example code for integrating with the Bloomit platform.

## Support & Resources

| Resource | Link |
|----------|------|
| 🌐 **Website** | [bloomit.com.ar](https://bloomit.com.ar) |
| 📱 **Web Dashboard** | [app.bloomit.com.ar](https://app.bloomit.com.ar) |
| 📧 **Developer Contact** | develop@bloomit.com.ar |
| 📚 **API Documentation** | See [API Reference](#api-reference) section |

For issues and questions:
- Check the [Troubleshooting](#troubleshooting) section
- Review API documentation
- Contact Bloomit support at [bloomit.com.ar](https://bloomit.com.ar)

