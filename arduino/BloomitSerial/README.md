# 🔌 Bloomit Serial Example - Beginner's Guide

Hello! 👋 This guide will help you connect your Arduino to Bloomit step by step, even if you've never done anything like this before.

## 📖 What does this code do?

This code makes your Arduino send sensor data (like temperature and humidity) to Bloomit through the USB cable. **You don't need WiFi** - it works with any Arduino connected to your computer.

### What is Bloomit?
Bloomit is an IoT platform that helps you connect your devices and sensors to the cloud. You can collect, monitor, and analyze sensor data from anywhere using your phone or computer.

## 🎯 What do I need?

### Physical things (Hardware)
- ✅ An Arduino (any one works: Uno, Nano, Mega, etc.)
- ✅ A USB cable to connect the Arduino to your computer
- ⚠️ **You DON'T need WiFi** - that's the great thing about this example

### Programs (Software)
- ✅ **Arduino IDE** - It's free, download it from [arduino.cc](https://www.arduino.cc/en/software)
- ✅ A Bloomit account (download the app on your phone)

## 🚀 Step by Step: How to Get Started

### Step 1: Install Arduino IDE

1. Go to [arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Download Arduino IDE (it's free)
3. Install it on your computer
4. Open it

### Step 2: Install the Required Library

A "library" is code that someone else wrote to help you. We need one called "ArduinoJson".

**How to do it:**
1. In Arduino IDE, go to the menu: **Tools → Manage Libraries...**
   - Or press `Ctrl+Shift+I` (Windows/Linux) or `Cmd+Shift+I` (Mac)
2. In the search box, type: `ArduinoJson`
3. Look for the one that says "ArduinoJson by Benoit Blanchon"
4. Click **Install** (install version 7.0.0 or newer)
5. Wait for it to finish installing

✅ **Done!** You now have the library installed.

### Step 3: Open the Code

1. In Arduino IDE, go to **File → Open**
2. Find and open the file `BloomitSerial.ino`
3. You'll see the code on the screen

### Step 4: Get Your Device Token

The "token" is like a special password that identifies your Arduino in Bloomit.

**How to get it:**
1. Open the Bloomit app on your phone
2. Go to the "Add Device" or "Link Device" section
3. The app will show you a long token (looks like: `eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...`)
4. **Copy that complete token**

### Step 5: Paste Your Token in the Code

1. In the code you opened, find this line (around line 26):
   ```cpp
   const char DEVICE_TOKEN[] = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...";
   ```
2. **Delete** the token that's there (everything between the quotes `"..."`)
3. **Paste** your token that you copied from the app
4. It should look like this:
   ```cpp
   const char DEVICE_TOKEN[] = "YOUR_TOKEN_HERE";
   ```

⚠️ **Important:** Don't delete the quotes `"` at the beginning and end.

### Step 6: Connect Your Arduino

1. Connect your Arduino to the computer with the USB cable
2. In Arduino IDE, go to **Tools → Board**
3. Select your Arduino type (for example: "Arduino Uno")
4. Go to **Tools → Port**
5. Select the port where your Arduino is connected
   - On Windows: something like `COM3` or `COM4`
   - On Mac/Linux: something like `/dev/ttyUSB0` or `/dev/ttyACM0`

### Step 7: Upload the Code

1. Click the **Upload** button (arrow pointing right) at the top
2. Or press `Ctrl+U` (Windows/Linux) or `Cmd+U` (Mac)
3. Wait for it to finish (you'll see "Done uploading" at the bottom)

✅ **Congratulations!** Your Arduino is now configured.

## 📱 How to Use It

### Option 1: Using bloomitweb Serial Proxy (Recommended - Easiest)

1. Connect your Arduino to your computer (you already did this)
2. Open bloomitweb in your browser
3. Use the "Serial Proxy" feature that bloomitweb has
4. Done! Your Arduino will automatically send data every 30 seconds

**What happens?**
- Your Arduino sends data through the USB cable
- bloomitweb receives it and sends it to Bloomit's cloud
- You can see the data in the Bloomit app

### Option 2: Using Python Bridge Script (Optional - For Advanced Users)

If you want to use the Python script instead of bloomitweb:

1. Open a terminal or console on your computer
2. Go to the folder where the code is:
   ```bash
   cd arduino/BloomitSerial
   ```
3. Install dependencies:
   ```bash
   pip3 install -r requirements.txt
   ```
4. Run the script:
   ```bash
   python3 serial_bridge.py
   ```

## 🔍 Check if It's Working

### Open Serial Monitor

The Serial Monitor shows you messages from your Arduino.

1. In Arduino IDE, go to **Tools → Serial Monitor**
   - Or press `Ctrl+Shift+M` (Windows/Linux) or `Cmd+Shift+M` (Mac)
2. In the bottom right corner, make sure it says **9600 baud**
3. You should see messages like:
   ```
   [OK] Arduino ready - sending sensor data to bloomitweb proxy
   ```

If you see these messages, everything is working! 🎉

## 📊 What Data Does It Send?

Your Arduino sends two types of information:

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

## ❓ Common Problems and Solutions

### Problem: "Port not found"

**Solution:**
- Disconnect and reconnect the USB cable
- In Arduino IDE, go to **Tools → Port** and select the correct port
- If you're using Windows, you might need to install Arduino drivers

### Problem: "Upload error"

**Solution:**
- Verify you selected the correct board in **Tools → Board**
- Verify you selected the correct port in **Tools → Port**
- Try disconnecting and reconnecting the USB cable
- Make sure no other program is using the serial port

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

### Problem: Token appears as "null"

**Solution:**
- Verify you copied the complete token (it's very long)
- Verify the quotes `"` are at the beginning and end
- Make sure there are no spaces before or after the token
- Copy the token again from the Bloomit app

### Problem: I don't see data in bloomitweb

**Solution:**
- Verify the Arduino is connected via USB
- Open Serial Monitor to see if there are error messages
- Verify bloomitweb is using the correct port
- Make sure the token is correct

## 📁 Files in This Folder

```
BloomitSerial/
├── BloomitSerial.ino    ← The main code (this is what you edit)
├── serial_bridge.py     ← Optional Python script (not needed if using bloomitweb)
├── requirements.txt     ← Python dependencies (only if using the script)
└── README.md           ← This file (the guide you're reading)
```

## 💡 Tips for Beginners

1. **Don't worry if it doesn't work the first time** - It's normal to have problems at first
2. **Read error messages** - They tell you what's wrong
3. **Use Serial Monitor** - It shows you what your Arduino is doing
4. **The token never expires** - Once you set it up, you don't need to change it
5. **You can disconnect and reconnect** - Your Arduino saves the code you uploaded

## 🎓 Next Steps

Once this works, you can:
- Connect real sensors (instead of using simulated data)
- Change the sending frequency
- Add more sensor types
- Customize log messages

## 📞 Need Help?

- Check Serial Monitor for error messages
- Verify all previous steps are complete
- Make sure the token is correct
- Try disconnecting and reconnecting everything

---

**Success! 🔌 Your Arduino is now connected to Bloomit.**
