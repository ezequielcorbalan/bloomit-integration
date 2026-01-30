/**
 * Bloomit Device API - Arduino Example
 * 
 * This example demonstrates how to interact with the Bloomit Device API
 * from any Arduino-compatible board with WiFi capability.
 * 
 * Authentication Flow:
 * 1. User logs in via user.api.bloomit.app -> receives user_token
 * 2. Device registers with user_token -> receives device_token (never expires)
 * 3. Device uses device_token for all subsequent API calls
 * 
 * Features:
 * - WiFi connection management
 * - Device registration with user token
 * - Sensor data submission (temperature, humidity)
 * - Device logging (info, warning, error, debug)
 * - Persistent storage for device token
 * 
 * Required Libraries (install via Library Manager):
 * - ArduinoJson
 * - ArduinoHttpClient
 * - WiFi library for your board
 * 
 * API Endpoints:
 * - POST /register/public - Register device (requires user_token)
 * - POST /sense - Submit sensor data (requires device_token)
 * - POST /device/log - Send log message (requires device_token)
 * - GET /api/device - Get device info (requires device_token)
 * 
 * Adapt the WiFi and storage sections for your specific board.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// =============================================================================
// Configuration - MODIFY THESE VALUES
// =============================================================================

// WiFi credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Bloomit API configuration
const char* API_HOST = "device.api.bloomit.app";
const char* API_BASE_URL = "https://device.api.bloomit.app";

// User token - obtained by logging in at user.api.bloomit.app
// This token is used ONLY for initial device registration
// After registration, the device receives a device_token that never expires
const char* USER_TOKEN = "YOUR_USER_TOKEN_HERE";

// =============================================================================
// Global Variables
// =============================================================================

Preferences preferences;
String deviceToken = "";
String deviceId = "";
String userId = "";

// Sensor simulation (replace with actual sensor readings)
float simulatedTemperature = 25.0;
float simulatedHumidity = 60.0;

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Generate device registration ID from MAC address
 * Format: REG-AABBCCDDEEFF
 */
String getDeviceRegistrationId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char regId[20];
  snprintf(regId, sizeof(regId), "REG-%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(regId);
}

/**
 * Load stored credentials from persistent storage
 * Adapt this function for your board's storage mechanism
 */
void loadCredentials() {
  preferences.begin("bloomit", true);
  deviceToken = preferences.getString("token", "");
  deviceId = preferences.getString("device_id", "");
  userId = preferences.getString("user_id", "");
  preferences.end();
  
  if (deviceToken.length() > 0) {
    Serial.println("[OK] Loaded stored device token");
    Serial.println("     Device ID: " + deviceId);
    Serial.println("     User ID: " + userId);
  } else {
    Serial.println("[INFO] No stored credentials found");
  }
}

/**
 * Save credentials to persistent storage
 * Adapt this function for your board's storage mechanism
 */
void saveCredentials() {
  preferences.begin("bloomit", false);
  preferences.putString("token", deviceToken);
  preferences.putString("device_id", deviceId);
  preferences.putString("user_id", userId);
  preferences.end();
  Serial.println("[OK] Credentials saved to storage");
}

/**
 * Clear stored credentials (for factory reset)
 */
void clearCredentials() {
  preferences.begin("bloomit", false);
  preferences.clear();
  preferences.end();
  deviceToken = "";
  deviceId = "";
  userId = "";
  Serial.println("[OK] All credentials cleared");
}

// =============================================================================
// WiFi Functions
// =============================================================================

/**
 * Connect to WiFi network
 */
bool connectToWiFi() {
  Serial.println("\n[WIFI] Connecting to WiFi...");
  Serial.println("       SSID: " + String(WIFI_SSID));
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi connected!");
    Serial.println("     IP Address: " + WiFi.localIP().toString());
    Serial.println("     MAC Address: " + WiFi.macAddress());
    return true;
  } else {
    Serial.println("\n[ERROR] WiFi connection failed!");
    return false;
  }
}

// =============================================================================
// Bloomit API Functions
// =============================================================================

/**
 * Register device with Bloomit API
 * 
 * Endpoint: POST /register/public
 * Body: { "userToken": "...", "device_registration_id": "REG-..." }
 * 
 * Returns: device_token (never expires) used for all subsequent API calls
 */
bool registerDevice() {
  if (deviceToken.length() > 0) {
    Serial.println("[INFO] Device already registered, skipping registration");
    return true;
  }
  
  Serial.println("\n[API] Registering device...");
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/register/public";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000);
  
  // Build registration payload
  JsonDocument doc;
  doc["userToken"] = USER_TOKEN;
  doc["device_registration_id"] = getDeviceRegistrationId();
  
  String payload;
  serializeJson(doc, payload);
  
  Serial.println("       URL: " + url);
  Serial.println("       Registration ID: " + getDeviceRegistrationId());
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("       HTTP Code: " + String(httpCode));
    
    if (httpCode == 201 || httpCode == 200) {
      JsonDocument responseDoc;
      DeserializationError error = deserializeJson(responseDoc, response);
      
      if (!error) {
        bool success = responseDoc["success"] | false;
        
        if (success) {
          deviceToken = responseDoc["device_token"].as<String>();
          
          JsonObject data = responseDoc["data"];
          deviceId = String(data["device_id"].as<int>());
          userId = String(data["user_id"].as<int>());
          
          saveCredentials();
          
          Serial.println("[OK] Device registered successfully!");
          Serial.println("     Device ID: " + deviceId);
          Serial.println("     User ID: " + userId);
          Serial.println("     Token received (never expires)");
          
          http.end();
          return true;
        } else {
          String errorMsg = responseDoc["error"] | "Unknown error";
          Serial.println("[ERROR] Registration failed: " + errorMsg);
        }
      } else {
        Serial.println("[ERROR] Failed to parse response JSON");
        Serial.println("        Response: " + response);
      }
    } else {
      Serial.println("[ERROR] Registration failed with HTTP code: " + String(httpCode));
      Serial.println("        Response: " + response);
    }
  } else {
    Serial.println("[ERROR] HTTP request failed: " + http.errorToString(httpCode));
  }
  
  http.end();
  return false;
}

/**
 * Send sensor data to Bloomit API
 * 
 * Endpoint: POST /sense
 * Headers: Authorization: Bearer <device_token>
 * Body: { "sensorType": "...", "value": "..." }
 */
bool sendSensorData(const String& sensorType, const String& value) {
  if (deviceToken.length() == 0) {
    Serial.println("[ERROR] Cannot send sensor data: device not registered");
    return false;
  }
  
  Serial.println("\n[API] Sending sensor data...");
  Serial.println("      Type: " + sensorType);
  Serial.println("      Value: " + value);
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/sense";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + deviceToken);
  http.setTimeout(10000);
  
  JsonDocument doc;
  doc["sensorType"] = sensorType;
  doc["value"] = value;
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    String response = http.getString();
    
    if (httpCode == 201 || httpCode == 200) {
      Serial.println("[OK] Sensor data sent successfully!");
      http.end();
      return true;
    } else {
      Serial.println("[ERROR] Failed to send sensor data. HTTP code: " + String(httpCode));
      Serial.println("        Response: " + response);
    }
  } else {
    Serial.println("[ERROR] HTTP request failed: " + http.errorToString(httpCode));
  }
  
  http.end();
  return false;
}

/**
 * Send log message to Bloomit API
 * 
 * Endpoint: POST /device/log
 * Headers: Authorization: Bearer <device_token>
 * Body: { "type": "info|warning|error|debug", "message": "..." }
 */
bool sendLog(const String& type, const String& message) {
  if (deviceToken.length() == 0) {
    Serial.println("[ERROR] Cannot send log: device not registered");
    return false;
  }
  
  Serial.println("\n[API] Sending log message...");
  Serial.println("      Type: " + type);
  Serial.println("      Message: " + message);
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/device/log";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + deviceToken);
  http.setTimeout(10000);
  
  JsonDocument doc;
  doc["type"] = type;
  doc["message"] = message;
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    String response = http.getString();
    
    if (httpCode == 201 || httpCode == 200) {
      Serial.println("[OK] Log sent successfully!");
      http.end();
      return true;
    } else {
      Serial.println("[ERROR] Failed to send log. HTTP code: " + String(httpCode));
      Serial.println("        Response: " + response);
    }
  } else {
    Serial.println("[ERROR] HTTP request failed: " + http.errorToString(httpCode));
  }
  
  http.end();
  return false;
}

/**
 * Get device information from Bloomit API
 * 
 * Endpoint: GET /api/device
 * Headers: Authorization: Bearer <device_token>
 */
bool getDeviceInfo() {
  if (deviceToken.length() == 0) {
    Serial.println("[ERROR] Cannot get device info: device not registered");
    return false;
  }
  
  Serial.println("\n[API] Getting device information...");
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/api/device";
  
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + deviceToken);
  http.setTimeout(10000);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String response = http.getString();
    
    if (httpCode == 200) {
      JsonDocument responseDoc;
      DeserializationError error = deserializeJson(responseDoc, response);
      
      if (!error && responseDoc["success"].as<bool>()) {
        JsonObject data = responseDoc["data"];
        
        Serial.println("[OK] Device info retrieved:");
        Serial.println("     Device ID: " + String(data["device_id"].as<int>()));
        Serial.println("     User ID: " + String(data["user_id"].as<int>()));
        Serial.println("     Is Active: " + String(data["is_active"].as<bool>() ? "Yes" : "No"));
        
        if (data.containsKey("last_time_connected")) {
          Serial.println("     Last Connected: " + data["last_time_connected"].as<String>());
        }
        
        http.end();
        return true;
      }
    }
    
    Serial.println("[ERROR] Failed to get device info. HTTP code: " + String(httpCode));
    Serial.println("        Response: " + response);
  } else {
    Serial.println("[ERROR] HTTP request failed: " + http.errorToString(httpCode));
  }
  
  http.end();
  return false;
}

// =============================================================================
// Convenience Functions (Log levels)
// =============================================================================

bool logInfo(const String& message) {
  return sendLog("info", message);
}

bool logWarning(const String& message) {
  return sendLog("warning", message);
}

bool logError(const String& message) {
  return sendLog("error", message);
}

bool logDebug(const String& message) {
  return sendLog("debug", message);
}

// =============================================================================
// Sensor Functions (Example implementations)
// =============================================================================

/**
 * Read temperature from sensor
 * Replace with actual sensor reading code for your hardware
 */
float readTemperature() {
  // Simulated reading - replace with your sensor code
  simulatedTemperature += random(-10, 11) / 10.0;
  simulatedTemperature = constrain(simulatedTemperature, 15.0, 35.0);
  return simulatedTemperature;
}

/**
 * Read humidity from sensor
 * Replace with actual sensor reading code for your hardware
 */
float readHumidity() {
  // Simulated reading - replace with your sensor code
  simulatedHumidity += random(-5, 6) / 10.0;
  simulatedHumidity = constrain(simulatedHumidity, 30.0, 90.0);
  return simulatedHumidity;
}

/**
 * Send all sensor readings to Bloomit API
 */
void sendAllSensorReadings() {
  float temp = readTemperature();
  float hum = readHumidity();
  
  Serial.println("\n[SENSOR] Current Readings:");
  Serial.println("         Temperature: " + String(temp, 2) + " C");
  Serial.println("         Humidity: " + String(hum, 2) + " %");
  
  sendSensorData("temperature", String(temp, 2));
  delay(500);
  sendSensorData("humidity", String(hum, 2));
}

// =============================================================================
// Main Setup and Loop
// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n");
  Serial.println("========================================================");
  Serial.println("        Bloomit Device API - Arduino Example");
  Serial.println("========================================================");
  Serial.println();
  
  // Load any stored credentials
  loadCredentials();
  
  // Connect to WiFi
  if (!connectToWiFi()) {
    Serial.println("[WARN] Could not connect to WiFi. Will retry...");
  }
  
  // Register device if not already registered
  if (WiFi.status() == WL_CONNECTED) {
    if (!registerDevice()) {
      Serial.println("[WARN] Device registration failed!");
      Serial.println("       Make sure USER_TOKEN is set correctly.");
      Serial.println("       The device will retry in the main loop.");
    }
    
    // If registered, send startup log
    if (deviceToken.length() > 0) {
      logInfo("Device started - Arduino Example");
      getDeviceInfo();
    }
  }
  
  Serial.println("\n========================================================");
  Serial.println("Setup complete. Starting main loop...");
  Serial.println("========================================================\n");
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WARN] WiFi disconnected. Reconnecting...");
    connectToWiFi();
    delay(5000);
    return;
  }
  
  // Try to register if not registered yet
  if (deviceToken.length() == 0) {
    registerDevice();
    delay(30000);
    return;
  }
  
  // Send sensor readings every 30 seconds
  static unsigned long lastSensorTime = 0;
  if (millis() - lastSensorTime > 30000 || lastSensorTime == 0) {
    lastSensorTime = millis();
    sendAllSensorReadings();
  }
  
  // Send heartbeat log every 5 minutes
  static unsigned long lastHeartbeatTime = 0;
  if (millis() - lastHeartbeatTime > 300000 || lastHeartbeatTime == 0) {
    lastHeartbeatTime = millis();
    logInfo("Heartbeat - Device running normally");
  }
  
  // Handle serial commands (optional)
  handleSerialCommands();
  
  delay(1000);
}

// =============================================================================
// Serial Command Interface
// =============================================================================

/**
 * Handle serial commands for debugging:
 * - "status" - Show device status
 * - "register" - Force re-registration
 * - "info" - Get device info from API
 * - "sense" - Send sensor data now
 * - "log <message>" - Send log message
 * - "clear" - Clear stored credentials
 */
void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "status") {
      Serial.println("\n[STATUS] Device Status:");
      Serial.println("         WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
      Serial.println("         IP: " + WiFi.localIP().toString());
      Serial.println("         Registered: " + String(deviceToken.length() > 0 ? "Yes" : "No"));
      Serial.println("         Device ID: " + deviceId);
      Serial.println("         User ID: " + userId);
    }
    else if (command == "register") {
      clearCredentials();
      registerDevice();
    }
    else if (command == "info") {
      getDeviceInfo();
    }
    else if (command == "sense") {
      sendAllSensorReadings();
    }
    else if (command.startsWith("log ")) {
      String message = command.substring(4);
      logInfo(message);
    }
    else if (command == "clear") {
      Serial.println("[INFO] Clearing credentials...");
      clearCredentials();
    }
    else if (command == "help") {
      Serial.println("\n[HELP] Available commands:");
      Serial.println("       status   - Show device status");
      Serial.println("       register - Force re-registration");
      Serial.println("       info     - Get device info from API");
      Serial.println("       sense    - Send sensor data now");
      Serial.println("       log <msg>- Send log message");
      Serial.println("       clear    - Clear stored credentials");
    }
    else if (command.length() > 0) {
      Serial.println("[ERROR] Unknown command: " + command);
      Serial.println("        Type 'help' for available commands");
    }
  }
}
