/**
 * Bloomit Device API - Arduino WiFi Example (ESP32/ESP8266)
 * 
 * This example sends sensor data directly to the Bloomit API via WiFi.
 * No host computer needed - the device connects to internet directly.
 * 
 * Requirements:
 * - ESP32 or ESP8266 board
 * - WiFi credentials configured
 * 
 * Required Libraries:
 * - ArduinoJson
 * - WiFi (built-in for ESP32/ESP8266)
 * - HTTPClient (built-in for ESP32/ESP8266)
 */

#include <ArduinoJson.h>

// Platform-specific includes
#if defined(ESP32)
  #include <WiFi.h>
  #include <HTTPClient.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClientSecure.h>
#else
  #error "This code requires ESP32 or ESP8266 board with WiFi"
#endif

// =============================================================================
// Configuration - MODIFY THESE VALUES
// =============================================================================

// WiFi credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Device token - provided by the Bloomit app
const char* DEVICE_TOKEN = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJkZXZpY2VfaWQiOiIxNDAiLCJpZCI6IjE0MCIsInVzZXJUb2tlbiI6ImV5SmhiR2NpT2lKSVV6STFOaUlzSW5SNWNDSTZJa3BYVkNKOS5leUoxYzJWeVgybGtJam8wTENKbGJXRnBiQ0k2SW1WNlpYRjFhV1ZzWTI5eVltRnNZVzVBTXpCbUxtTnZiUzVoY2lJc0ltbGhkQ0k2TVRjMk9UZ3dNRGs1TkN3aVpYaHdJam94Tnpjd05EQTFOemswZlEuRmoxODVCWkstMzdXcTM4ZHRKUnNJTngyd3BrU25Tc3JlLU1lNXdWZzBaZyIsImlhdCI6MTc2OTgwMTAzOH0.G2KWG3mL0dXIkw_WDjdFiUUcZo7w0ZuvuBISp1bmcm8";

// API configuration
const char* API_BASE_URL = "https://device.api.bloomit.app";

// Sensor interval (milliseconds)
const unsigned long SENSOR_INTERVAL = 30000; // 30 seconds

// =============================================================================
// Global Variables
// =============================================================================

float simulatedTemperature = 25.0;
float simulatedHumidity = 60.0;
bool wifiConnected = false;

// =============================================================================
// WiFi Functions
// =============================================================================

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return;
  }
  
  Serial.print(F("Connecting to WiFi: "));
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.print(F("[OK] WiFi connected! IP: "));
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println(F("[ERROR] WiFi connection failed"));
  }
}

// =============================================================================
// HTTP Functions
// =============================================================================

bool sendHTTPRequest(const char* endpoint, const char* jsonBody) {
  if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[ERROR] WiFi not connected"));
    return false;
  }
  
  #if defined(ESP32)
    HTTPClient http;
    http.begin(String(API_BASE_URL) + endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(DEVICE_TOKEN));
    
    int httpCode = http.POST(jsonBody);
    
    bool success = false;
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
        String response = http.getString();
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, response);
        if (!error && doc["success"]) {
          success = true;
        }
      }
    }
    
    http.end();
    return success;
    
  #elif defined(ESP8266)
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation
    
    HTTPClient http;
    http.begin(client, String(API_BASE_URL) + endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(DEVICE_TOKEN));
    
    int httpCode = http.POST(jsonBody);
    
    bool success = false;
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
        String response = http.getString();
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, response);
        if (!error && doc["success"]) {
          success = true;
        }
      }
    }
    
    http.end();
    return success;
  #endif
}

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
// Send sensor data to API
// =============================================================================

bool sendSensorData(const char* sensorType, float value, const char* text = nullptr) {
  Serial.print(F("[API] Sending "));
  Serial.print(sensorType);
  Serial.print(F(": "));
  Serial.println(value);
  
  StaticJsonDocument<512> doc;
  doc["cmd"] = "sense";
  doc["sensorType"] = sensorType;
  doc["value"] = value;
  
  // Add optional text field if provided
  if (text != nullptr && strlen(text) > 0) {
    doc["text"] = text;
  }
  
  char jsonBody[512];
  serializeJson(doc, jsonBody);
  
  bool success = sendHTTPRequest("/sense", jsonBody);
  
  if (success) {
    Serial.println(F("[OK] Data sent successfully!"));
  } else {
    Serial.println(F("[ERROR] Failed to send data"));
  }
  
  return success;
}

// =============================================================================
// Send log message to API
// =============================================================================

bool sendLog(const char* type, const char* message) {
  Serial.print(F("[LOG] "));
  Serial.print(type);
  Serial.print(F(": "));
  Serial.println(message);
  
  StaticJsonDocument<512> doc;
  doc["type"] = type;  // "info", "warning", "error", "debug"
  doc["message"] = message;
  
  char jsonBody[512];
  serializeJson(doc, jsonBody);
  
  bool success = sendHTTPRequest("/device/log", jsonBody);
  
  if (success) {
    Serial.println(F("[OK] Log sent successfully!"));
  } else {
    Serial.println(F("[ERROR] Failed to send log"));
  }
  
  return success;
}

void sendAllSensorReadings() {
  float temp = readTemperature();
  float hum = readHumidity();
  
  Serial.println(F("\n[SENSOR] Current Readings:"));
  Serial.print(F("         Temperature: "));
  Serial.print(temp);
  Serial.println(F(" C"));
  Serial.print(F("         Humidity: "));
  Serial.print(hum);
  Serial.println(F(" %"));
  
  // Send with optional text
  sendSensorData("temperature", temp, "Temperature reading");
  delay(500);
  sendSensorData("humidity", hum, "Humidity reading");
}

// =============================================================================
// Main Setup and Loop
// =============================================================================

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("\n"));
  Serial.println(F("========================================================"));
  Serial.println(F("   Bloomit Device API - WiFi Example"));
  Serial.println(F("========================================================"));
  Serial.println();
  
  // Connect to WiFi
  connectWiFi();
  
  // Send startup log
  if (wifiConnected) {
    sendLog("info", "Device started - Arduino WiFi Example");
  }
  
  Serial.println(F("\nSetup complete. Starting main loop...\n"));
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    Serial.println(F("[WARN] WiFi disconnected, reconnecting..."));
    connectWiFi();
    delay(5000);
    return;
  }
  
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
