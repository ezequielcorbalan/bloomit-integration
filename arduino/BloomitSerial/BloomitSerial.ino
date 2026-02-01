/**
 * Bloomit Device API - Arduino Serial Example (No WiFi)
 * 
 * This example sends sensor data via USB/Serial to bloomitweb serial proxy.
 * Format: {"token":"YOUR_DEVICE_TOKEN","cmd":"sense","sensorType":1,"value":45.5,"text":"optional message"}
 * Log format: {"token":"YOUR_DEVICE_TOKEN","cmd":"log","type":"info","message":"text"}
 * 
 * Works with ANY Arduino board (no WiFi required)
 * 
 * Required Libraries:
 * - ArduinoJson
 * 
 * Sensor Types:
 * - 1 = Temperature
 * - 2 = Humidity
 */

#include <ArduinoJson.h>

// =============================================================================
// Configuration
// =============================================================================

// Device token - provided by the Bloomit app
// Use char array instead of const char* to avoid serialization issues
const char DEVICE_TOKEN[] = "DEVICE_TOKEN_HERE";

// Sensor interval (milliseconds)
const unsigned long SENSOR_INTERVAL = 30000; // 30 seconds

// =============================================================================
// Global Variables
// =============================================================================

float simulatedTemperature = 25.0;
float simulatedHumidity = 60.0;

// =============================================================================
// Sensor Functions (replace with real sensors)
// =============================================================================

float readTemperature() {
  simulatedTemperature += random(-10, 11) / 10.0;
  simulatedTemperature = constrain(simulatedTemperature, 15.0, 35.0);
  return simulatedTemperature;
}

float readHumidity() {
  simulatedHumidity += random(-5, 6) / 10.0;
  simulatedHumidity = constrain(simulatedHumidity, 30.0, 90.0);
  return simulatedHumidity;
}

// =============================================================================
// Send sensor data via Serial (JSON format for bloomitweb proxy)
// Format: {"token":"...","cmd":"sense","sensorType":1,"value":45.5,"text":"optional message"}
// =============================================================================

void sendSensorData(int sensorType, float value, const char* text = nullptr) {
  StaticJsonDocument<512> doc;
  
  doc["token"] = DEVICE_TOKEN;
  doc["cmd"] = "sense";
  doc["sensorType"] = sensorType;  // 1=temperature, 2=humidity
  doc["value"] = value;
  
  // Add optional text field if provided
  if (text != nullptr && strlen(text) > 0) {
    doc["text"] = text;
  }
  
  serializeJson(doc, Serial);
  Serial.println();  // End of message
}

// =============================================================================
// Send log message via Serial
// Format: {"token":"...","cmd":"log","type":"info","message":"text"}
// =============================================================================

void sendLog(const char* type, const char* message) {
  StaticJsonDocument<512> doc;
  
  doc["token"] = DEVICE_TOKEN;
  doc["cmd"] = "log";
  doc["type"] = type;  // "info", "warning", "error", "debug"
  doc["message"] = message;
  
  serializeJson(doc, Serial);
  Serial.println();  // End of message
}

void sendAllSensorReadings() {
  float temp = readTemperature();
  float hum = readHumidity();
  
  // Send temperature (sensorType: 1) with optional text
  sendSensorData(1, temp, "Temperature reading");
  delay(100);
  
  // Send humidity (sensorType: 2) with optional text
  sendSensorData(2, hum, "Humidity reading");
}

// =============================================================================
// Main Setup and Loop
// =============================================================================

void setup() {
  Serial.begin(9600);
  delay(2000);  // Wait for serial connection
  
  Serial.println(F("[OK] Arduino ready - sending sensor data to bloomitweb proxy"));
  
  // Send startup log
  sendLog("info", "Device started - Arduino Serial Example");
}

void loop() {
  static unsigned long lastSensorTime = 0;
  static unsigned long lastLogTime = 0;
  
  // Send sensor readings at interval
  if (millis() - lastSensorTime > SENSOR_INTERVAL || lastSensorTime == 0) {
    lastSensorTime = millis();
    sendAllSensorReadings();
  }
  
  // Send heartbeat log every 5 minutes
  if (millis() - lastLogTime > 300000 || lastLogTime == 0) {
    lastLogTime = millis();
    sendLog("info", "Heartbeat - Device running normally");
  }
  
  delay(1000);
}
