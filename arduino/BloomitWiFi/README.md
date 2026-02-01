# 🔌 Bloomit WiFi Example - Beginner's Guide

Hello! 👋 This guide will help you connect your ESP32 or ESP8266 to Bloomit using WiFi. It's more advanced than the Serial example, but still easy to follow.

## 📖 What does this code do?

This code makes your ESP32 or ESP8266 connect to the internet via WiFi and send sensor data directly to Bloomit. **You don't need a computer connected** - once configured, it works on its own.

### What is ESP32/ESP8266?
They are boards similar to Arduino but with built-in WiFi. They're perfect for IoT (Internet of Things) projects.

### What is Bloomit?
Bloomit is an IoT platform that helps you connect your devices and sensors to the cloud. You can collect, monitor, and analyze sensor data from anywhere using your phone or computer.

## 🎯 What do I need?

### Physical things (Hardware)
- ✅ An **ESP32** or **ESP8266** (you can buy them online, they're affordable)
- ✅ A USB cable to connect the board to your computer (only the first time)
- ✅ A WiFi connection (your home WiFi works perfectly)

### Programs (Software)
- ✅ **Arduino IDE** - It's free, download it from [arduino.cc](https://www.arduino.cc/en/software)
- ✅ A Bloomit account (download the app on your phone)

## 🚀 Step by Step: How to Get Started

### Step 1: Install Arduino IDE

1. Go to [arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Download Arduino IDE (it's free)
3. Install it on your computer
4. Open it

### Step 2: Add Support for ESP32/ESP8266

Arduino IDE doesn't come with support for ESP32/ESP8266 by default. We need to add it.

#### For ESP32:

1. In Arduino IDE, go to **File → Preferences**
2. In "Additional Boards Manager URLs", paste this URL:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Click **OK**
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32"
6. Install "esp32 by Espressif Systems"
7. Wait for it to finish (it may take several minutes)

#### For ESP8266:

1. In Arduino IDE, go to **File → Preferences**
2. In "Additional Boards Manager URLs", paste this URL:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Click **OK**
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp8266"
6. Install "esp8266 by ESP8266 Community"
7. Wait for it to finish

✅ **Done!** You can now use ESP32/ESP8266 in Arduino IDE.

### Step 3: Install the Required Library

A "library" is code that someone else wrote to help you. We need one called "ArduinoJson".

**How to do it:**
1. In Arduino IDE, go to the menu: **Tools → Manage Libraries...**
   - Or press `Ctrl+Shift+I` (Windows/Linux) or `Cmd+Shift+I` (Mac)
2. In the search box, type: `ArduinoJson`
3. Look for the one that says "ArduinoJson by Benoit Blanchon"
4. Click **Install** (install version 7.0.0 or newer)
5. Wait for it to finish installing

✅ **Done!** You now have the library installed.

### Step 4: Open the Code

1. In Arduino IDE, go to **File → Open**
2. Find and open the file `BloomitWiFi.ino`
3. You'll see the code on the screen

### Step 5: Configure Your WiFi

Your ESP32/ESP8266 needs to know how to connect to your WiFi.

1. In the code, find these lines (around lines 36-37):
   ```cpp
   const char* WIFI_SSID = "YOUR_WIFI_SSID";
   const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   ```

2. **Replace `YOUR_WIFI_SSID`** with your WiFi name:
   ```cpp
   const char* WIFI_SSID = "MyWiFi";
   ```

3. **Replace `YOUR_WIFI_PASSWORD`** with your WiFi password:
   ```cpp
   const char* WIFI_PASSWORD = "my_password_123";
   ```

⚠️ **Important:** 
- Don't delete the quotes `"` at the beginning and end
- The WiFi name is what you see when searching for WiFi networks on your phone
- The password is what you use to connect to WiFi

### Step 6: Get Your Device Token

The "token" is like a special password that identifies your device in Bloomit.

**How to get it:**
1. Open the Bloomit app on your phone
2. Go to the "Add Device" or "Link Device" section
3. The app will show you a long token (looks like: `eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...`)
4. **Copy that complete token**

### Step 7: Paste Your Token in the Code

1. In the code, find this line (around line 40):
   ```cpp
   const char* DEVICE_TOKEN = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...";
   ```
2. **Delete** the token that's there (everything between the quotes `"..."`)
3. **Paste** your token that you copied from the app
4. It should look like this:
   ```cpp
   const char* DEVICE_TOKEN = "YOUR_TOKEN_HERE";
   ```

⚠️ **Important:** Don't delete the quotes `"` at the beginning and end.

### Step 8: Select Your Board

1. Connect your ESP32 or ESP8266 to the computer with the USB cable
2. In Arduino IDE, go to **Tools → Board**

**If you have ESP32:**
- Select: **ESP32 Arduino → ESP32 Dev Module**

**If you have ESP8266:**
- Select: **ESP8266 Boards → NodeMCU 1.0 (ESP-12E Module)**
  - Or search for your specific model if you know it

### Step 9: Select the Port

1. In Arduino IDE, go to **Tools → Port**
2. Select the port where your device is connected
   - On Windows: something like `COM3` or `COM4`
   - On Mac/Linux: something like `/dev/ttyUSB0` or `/dev/ttyACM0`
   - If you don't see any port, disconnect and reconnect the USB cable

### Step 10: Upload the Code

1. Click the **Upload** button (arrow pointing right) at the top
2. Or press `Ctrl+U` (Windows/Linux) or `Cmd+U` (Mac)
3. Wait for it to finish (it may take longer than with a normal Arduino)
4. You'll see "Done uploading" at the bottom

✅ **Congratulations!** Your ESP32/ESP8266 is now configured.

## 📱 How It Works

Once you've uploaded the code:

1. Your device automatically connects to your WiFi
2. Once connected, it sends a startup message to Bloomit
3. Every 30 seconds, it sends sensor data (temperature and humidity)
4. Every 5 minutes, it sends a message to confirm it's still working

**You don't need to have the computer connected!** Once configured, it works on its own.

## 🔍 Check if It's Working

### Open Serial Monitor

The Serial Monitor shows you messages from your device.

1. In Arduino IDE, go to **Tools → Serial Monitor**
   - Or press `Ctrl+Shift+M` (Windows/Linux) or `Cmd+Shift+M` (Mac)
2. In the bottom right corner, make sure it says **9600 baud**
3. You should see messages like:
   ```
   Connecting to WiFi: MyWiFi
   ................
   [OK] WiFi connected! IP: 192.168.1.100
   [OK] Device started - Arduino WiFi Example
   [API] Sending temperature: 25.5
   [OK] Data sent successfully!
   ```

If you see these messages, everything is working! 🎉

### View Data in Bloomit

1. Open the Bloomit app on your phone
2. Go to your device section
3. You should see temperature and humidity data updating

## 📊 What Data Does It Send?

Your device sends two types of information:

### 1. Sensor Data
- **Temperature** (every 30 seconds)
- **Humidity** (every 30 seconds)

### 2. Log Messages
- A message when it starts
- A message every 5 minutes to confirm it's still working

## ⚙️ Customize (Optional)

### Change the Sending Frequency

By default, it sends data every 30 seconds. If you want to change it:

1. Find this line in the code:
   ```cpp
   const unsigned long SENSOR_INTERVAL = 30000; // 30 seconds
   ```
2. Change `30000` to the time you want (in milliseconds):
   - 10 seconds = `10000`
   - 1 minute = `60000`
   - 5 minutes = `300000`

## 🎮 Serial Monitor Commands

You can type commands in Serial Monitor to control your device:

- Type `status` and press Enter → Shows WiFi and token status
- Type `sense` and press Enter → Sends sensor data immediately
- Type `wifi` and press Enter → Tries to reconnect to WiFi
- Type `help` and press Enter → Shows all available commands

## ❓ Common Problems and Solutions

### Problem: "WiFi connection failed"

**Solution:**
- Verify the WiFi name (`WIFI_SSID`) is correct (exactly as it appears on your phone)
- Verify the password (`WIFI_PASSWORD`) is correct
- Make sure your WiFi is 2.4GHz (ESP32/ESP8266 don't support 5GHz)
- Move the device closer to the WiFi router for better signal
- Check Serial Monitor for more detailed error messages

### Problem: "Port not found"

**Solution:**
- Disconnect and reconnect the USB cable
- In Arduino IDE, go to **Tools → Port** and select the correct port
- If you're using Windows, you might need to install ESP32/ESP8266 drivers
- Try a different USB cable (some cables only charge, don't transmit data)

### Problem: "Upload error" or "Failed to connect"

**Solution:**
- Hold down the BOOT (or FLASH) button on your ESP32/ESP8266 while uploading
- Verify you selected the correct board in **Tools → Board**
- Verify you selected the correct port in **Tools → Port**
- Try disconnecting and reconnecting the USB cable
- Some ESP32/ESP8266 need you to press the RESET button after connecting

### Problem: Device keeps restarting

**Solution:**
- Verify the USB cable provides enough power (use a good quality cable)
- Verify your computer's USB port works well
- Try using a USB power adapter (like from a phone) instead of the computer's USB port
- Reduce the data sending frequency (change `SENSOR_INTERVAL` to a larger number)

### Problem: "HTTP request failed"

**Solution:**
- Verify the token is correct (copy it again from the app)
- Verify your device is connected to the internet (check Serial Monitor)
- Make sure your WiFi has internet (try opening a webpage)
- Check Serial Monitor for the specific HTTP error code

### Problem: Token appears as "null"

**Solution:**
- Verify you copied the complete token (it's very long)
- Verify the quotes `"` are at the beginning and end
- Make sure there are no spaces before or after the token
- Copy the token again from the Bloomit app

### Problem: "Permission denied" (Linux/Mac)

**Solution:**
Open a terminal and run:
```bash
sudo chmod 666 /dev/ttyACM0
```
(Replace `/dev/ttyACM0` with the port shown in Arduino IDE)

Or add your user to the dialout group:
```bash
sudo usermod -a -G dialout $USER
```
Then log out and log back in.

## 📁 Files in This Folder

```
BloomitWiFi/
├── BloomitWiFi.ino    ← The main code (this is what you edit)
└── README.md         ← This file (the guide you're reading)
```

## 💡 Tips for Beginners

1. **Don't worry if it doesn't work the first time** - It's normal to have problems at first
2. **Read messages in Serial Monitor** - They tell you exactly what's happening
3. **WiFi must be 2.4GHz** - Most ESP32/ESP8266 don't support 5GHz WiFi
4. **The token never expires** - Once you set it up, you don't need to change it
5. **You can disconnect the computer** - Once configured, it works on its own
6. **Use a good quality USB cable** - Some cheap cables don't work well

## 🎓 Next Steps

Once this works, you can:
- Connect real sensors (instead of using simulated data)
- Change the sending frequency
- Add more sensor types
- Customize log messages
- Place the device anywhere with WiFi (doesn't need to be near the computer)

## 📞 Need Help?

- Check Serial Monitor for detailed error messages
- Verify all previous steps are complete
- Make sure WiFi and token are correct
- Try disconnecting and reconnecting everything
- Verify your WiFi has working internet

---

**Success! 🔌 Your ESP32/ESP8266 is now connected to Bloomit and working autonomously.**
