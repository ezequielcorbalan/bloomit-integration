/**
 * Bloomit Device API - ESP32 (ESP-IDF) Example
 * 
 * This example demonstrates how to interact with the Bloomit Device API
 * using ESP-IDF framework.
 * 
 * Authentication Flow:
 * 1. User logs in via user.api.bloomit.app -> receives user_token
 * 2. Device registers with user_token -> receives device_token (never expires)
 * 3. Device uses device_token for all subsequent API calls
 * 
 * Features:
 * - WiFi station mode with event-driven architecture
 * - Secure HTTPS communication
 * - Device registration with user token
 * - Sensor data submission (temperature, humidity)
 * - Device logging (info, warning, error, debug)
 * - NVS persistent storage for device token
 * 
 * API Endpoints:
 * - POST /register/public - Register device (requires user_token)
 * - POST /sense - Submit sensor data (requires device_token)
 * - POST /device/log - Send log message (requires device_token)
 * - GET /api/device - Get device info (requires device_token)
 * 
 * Compatible targets: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "cJSON.h"

// =============================================================================
// Configuration - MODIFY THESE VALUES
// =============================================================================

// WiFi credentials
#define WIFI_SSID           "YOUR_WIFI_SSID"
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"
#define WIFI_MAX_RETRY      10

// Bloomit API configuration
#define API_BASE_URL        "https://device.api.bloomit.app"

// User token - obtained by logging in at user.api.bloomit.app
// This token is used ONLY for initial device registration
// After registration, the device receives a device_token that never expires
#define USER_TOKEN          "YOUR_USER_TOKEN_HERE"

// NVS namespace and keys
#define NVS_NAMESPACE       "bloomit"
#define NVS_KEY_TOKEN       "device_token"
#define NVS_KEY_DEVICE_ID   "device_id"
#define NVS_KEY_USER_ID     "user_id"

// Buffer sizes
#define HTTP_RESPONSE_BUFFER_SIZE   2048
#define MAX_TOKEN_SIZE              512
#define MAX_HTTP_RECV_BUFFER        512

// =============================================================================
// Global Variables
// =============================================================================

static const char *TAG = "bloomit_example";

// WiFi event group
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static int s_retry_num = 0;

// Device credentials
static char s_device_token[MAX_TOKEN_SIZE] = {0};
static int32_t s_device_id = 0;
static int32_t s_user_id = 0;

// HTTP response buffer
static char s_http_response[HTTP_RESPONSE_BUFFER_SIZE];
static int s_http_response_len = 0;

// =============================================================================
// HTTP Event Handler
// =============================================================================

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (s_http_response_len + evt->data_len < HTTP_RESPONSE_BUFFER_SIZE - 1) {
                memcpy(s_http_response + s_http_response_len, evt->data, evt->data_len);
                s_http_response_len += evt->data_len;
                s_http_response[s_http_response_len] = '\0';
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Generate device registration ID from MAC address
 * Format: REG-AABBCCDDEEFF
 */
static void get_device_registration_id(char *reg_id, size_t max_len) {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(reg_id, max_len, "REG-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/**
 * Load stored credentials from NVS
 */
static esp_err_t load_credentials(void) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No stored credentials found (NVS namespace doesn't exist)");
        return err;
    }

    size_t len = sizeof(s_device_token);
    err = nvs_get_str(nvs, NVS_KEY_TOKEN, s_device_token, &len);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No device token in NVS");
        nvs_close(nvs);
        return err;
    }

    nvs_get_i32(nvs, NVS_KEY_DEVICE_ID, &s_device_id);
    nvs_get_i32(nvs, NVS_KEY_USER_ID, &s_user_id);

    nvs_close(nvs);

    ESP_LOGI(TAG, "✅ Loaded stored credentials");
    ESP_LOGI(TAG, "   Device ID: %ld", (long)s_device_id);
    ESP_LOGI(TAG, "   User ID: %ld", (long)s_user_id);

    return ESP_OK;
}

/**
 * Save credentials to NVS
 */
static esp_err_t save_credentials(void) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %s", esp_err_to_name(err));
        return err;
    }

    nvs_set_str(nvs, NVS_KEY_TOKEN, s_device_token);
    nvs_set_i32(nvs, NVS_KEY_DEVICE_ID, s_device_id);
    nvs_set_i32(nvs, NVS_KEY_USER_ID, s_user_id);

    err = nvs_commit(nvs);
    nvs_close(nvs);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✅ Credentials saved to NVS");
    }

    return err;
}

/**
 * Clear stored credentials
 */
static esp_err_t clear_credentials(void) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        nvs_erase_all(nvs);
        nvs_commit(nvs);
        nvs_close(nvs);
    }

    memset(s_device_token, 0, sizeof(s_device_token));
    s_device_id = 0;
    s_user_id = 0;

    ESP_LOGI(TAG, "🗑️ All credentials cleared");
    return ESP_OK;
}

// =============================================================================
// WiFi Functions
// =============================================================================

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying WiFi connection (%d/%d)...", s_retry_num, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "WiFi connection failed");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "✅ WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "📶 Connecting to WiFi SSID: %s", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "❌ Failed to connect to WiFi");
        return ESP_FAIL;
    }

    return ESP_FAIL;
}

// =============================================================================
// Bloomit API Functions
// =============================================================================

/**
 * Register device with Bloomit API
 * 
 * Endpoint: POST /register/public
 * Body: { "userToken": "...", "device_registration_id": "REG-..." }
 */
static esp_err_t bloomit_register_device(void) {
    if (strlen(s_device_token) > 0) {
        ESP_LOGI(TAG, "ℹ️ Device already registered, skipping registration");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "\n📝 Registering device with Bloomit API...");

    // Build registration ID
    char reg_id[32];
    get_device_registration_id(reg_id, sizeof(reg_id));
    ESP_LOGI(TAG, "   Registration ID: %s", reg_id);

    // Build JSON payload
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "userToken", USER_TOKEN);
    cJSON_AddStringToObject(json, "device_registration_id", reg_id);
    char *json_string = cJSON_Print(json);
    
    if (!json_string) {
        cJSON_Delete(json);
        ESP_LOGE(TAG, "Failed to create JSON payload");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "   Payload: %s", json_string);

    // Clear response buffer
    memset(s_http_response, 0, sizeof(s_http_response));
    s_http_response_len = 0;

    // Configure HTTP client
    char url[128];
    snprintf(url, sizeof(url), "%s/register/public", API_BASE_URL);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 15000,
        .disable_auto_redirect = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));

    esp_err_t err = esp_http_client_perform(client);

    esp_err_t result = ESP_FAIL;

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "   HTTP Status: %d", status_code);

        if (status_code == 200 || status_code == 201) {
            // Parse response
            cJSON *response = cJSON_Parse(s_http_response);
            if (response) {
                cJSON *success = cJSON_GetObjectItem(response, "success");
                if (success && cJSON_IsTrue(success)) {
                    // Extract device token
                    cJSON *token = cJSON_GetObjectItem(response, "device_token");
                    if (token && cJSON_IsString(token)) {
                        strncpy(s_device_token, token->valuestring, sizeof(s_device_token) - 1);
                    }

                    // Extract device info
                    cJSON *data = cJSON_GetObjectItem(response, "data");
                    if (data) {
                        cJSON *device_id = cJSON_GetObjectItem(data, "device_id");
                        if (device_id && cJSON_IsNumber(device_id)) {
                            s_device_id = (int32_t)cJSON_GetNumberValue(device_id);
                        }

                        cJSON *user_id = cJSON_GetObjectItem(data, "user_id");
                        if (user_id && cJSON_IsNumber(user_id)) {
                            s_user_id = (int32_t)cJSON_GetNumberValue(user_id);
                        }
                    }

                    // Save credentials
                    save_credentials();

                    ESP_LOGI(TAG, "✅ Device registered successfully!");
                    ESP_LOGI(TAG, "   Device ID: %ld", (long)s_device_id);
                    ESP_LOGI(TAG, "   User ID: %ld", (long)s_user_id);

                    result = ESP_OK;
                } else {
                    cJSON *error = cJSON_GetObjectItem(response, "error");
                    ESP_LOGE(TAG, "❌ Registration failed: %s", 
                             error ? error->valuestring : "Unknown error");
                }
                cJSON_Delete(response);
            } else {
                ESP_LOGE(TAG, "❌ Failed to parse response JSON");
            }
        } else {
            ESP_LOGE(TAG, "❌ Registration failed with status: %d", status_code);
            ESP_LOGE(TAG, "   Response: %s", s_http_response);
        }
    } else {
        ESP_LOGE(TAG, "❌ HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(json);
    free(json_string);

    return result;
}

/**
 * Send sensor data to Bloomit API
 * 
 * Endpoint: POST /sense
 * Headers: Authorization: Bearer <device_token>
 * Body: { "sensorType": "...", "value": "..." }
 */
static esp_err_t bloomit_send_sensor_data(const char *sensor_type, const char *value) {
    if (strlen(s_device_token) == 0) {
        ESP_LOGE(TAG, "❌ Cannot send sensor data: device not registered");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "\n📊 Sending sensor data...");
    ESP_LOGI(TAG, "   Type: %s", sensor_type);
    ESP_LOGI(TAG, "   Value: %s", value);

    // Build JSON payload
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "sensorType", sensor_type);
    cJSON_AddStringToObject(json, "value", value);
    char *json_string = cJSON_Print(json);

    if (!json_string) {
        cJSON_Delete(json);
        return ESP_ERR_NO_MEM;
    }

    // Clear response buffer
    memset(s_http_response, 0, sizeof(s_http_response));
    s_http_response_len = 0;

    // Configure HTTP client
    char url[128];
    snprintf(url, sizeof(url), "%s/sense", API_BASE_URL);

    char auth_header[MAX_TOKEN_SIZE + 16];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", s_device_token);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_post_field(client, json_string, strlen(json_string));

    esp_err_t err = esp_http_client_perform(client);
    esp_err_t result = ESP_FAIL;

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200 || status_code == 201) {
            ESP_LOGI(TAG, "✅ Sensor data sent successfully!");
            result = ESP_OK;
        } else {
            ESP_LOGE(TAG, "❌ Failed to send sensor data. Status: %d", status_code);
            ESP_LOGE(TAG, "   Response: %s", s_http_response);
        }
    } else {
        ESP_LOGE(TAG, "❌ HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(json);
    free(json_string);

    return result;
}

/**
 * Send log message to Bloomit API
 * 
 * Endpoint: POST /device/log
 * Headers: Authorization: Bearer <device_token>
 * Body: { "type": "info|warning|error|debug", "message": "..." }
 */
static esp_err_t bloomit_send_log(const char *type, const char *message) {
    if (strlen(s_device_token) == 0) {
        ESP_LOGE(TAG, "❌ Cannot send log: device not registered");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "\n📝 Sending log message...");
    ESP_LOGI(TAG, "   Type: %s", type);
    ESP_LOGI(TAG, "   Message: %s", message);

    // Build JSON payload
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", type);
    cJSON_AddStringToObject(json, "message", message);
    char *json_string = cJSON_Print(json);

    if (!json_string) {
        cJSON_Delete(json);
        return ESP_ERR_NO_MEM;
    }

    // Clear response buffer
    memset(s_http_response, 0, sizeof(s_http_response));
    s_http_response_len = 0;

    // Configure HTTP client
    char url[128];
    snprintf(url, sizeof(url), "%s/device/log", API_BASE_URL);

    char auth_header[MAX_TOKEN_SIZE + 16];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", s_device_token);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_post_field(client, json_string, strlen(json_string));

    esp_err_t err = esp_http_client_perform(client);
    esp_err_t result = ESP_FAIL;

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200 || status_code == 201) {
            ESP_LOGI(TAG, "✅ Log sent successfully!");
            result = ESP_OK;
        } else {
            ESP_LOGE(TAG, "❌ Failed to send log. Status: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "❌ HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(json);
    free(json_string);

    return result;
}

/**
 * Get device information from Bloomit API
 * 
 * Endpoint: GET /api/device
 * Headers: Authorization: Bearer <device_token>
 */
static esp_err_t bloomit_get_device_info(void) {
    if (strlen(s_device_token) == 0) {
        ESP_LOGE(TAG, "❌ Cannot get device info: device not registered");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "\n📱 Getting device information...");

    // Clear response buffer
    memset(s_http_response, 0, sizeof(s_http_response));
    s_http_response_len = 0;

    // Configure HTTP client
    char url[128];
    snprintf(url, sizeof(url), "%s/api/device", API_BASE_URL);

    char auth_header[MAX_TOKEN_SIZE + 16];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", s_device_token);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "Authorization", auth_header);

    esp_err_t err = esp_http_client_perform(client);
    esp_err_t result = ESP_FAIL;

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            cJSON *response = cJSON_Parse(s_http_response);
            if (response) {
                cJSON *success = cJSON_GetObjectItem(response, "success");
                if (success && cJSON_IsTrue(success)) {
                    cJSON *data = cJSON_GetObjectItem(response, "data");
                    if (data) {
                        ESP_LOGI(TAG, "✅ Device info retrieved:");
                        
                        cJSON *device_id = cJSON_GetObjectItem(data, "device_id");
                        if (device_id) {
                            ESP_LOGI(TAG, "   Device ID: %d", (int)cJSON_GetNumberValue(device_id));
                        }
                        
                        cJSON *user_id = cJSON_GetObjectItem(data, "user_id");
                        if (user_id) {
                            ESP_LOGI(TAG, "   User ID: %d", (int)cJSON_GetNumberValue(user_id));
                        }
                        
                        cJSON *is_active = cJSON_GetObjectItem(data, "is_active");
                        if (is_active) {
                            ESP_LOGI(TAG, "   Is Active: %s", cJSON_IsTrue(is_active) ? "Yes" : "No");
                        }
                        
                        cJSON *last_connected = cJSON_GetObjectItem(data, "last_time_connected");
                        if (last_connected && cJSON_IsString(last_connected)) {
                            ESP_LOGI(TAG, "   Last Connected: %s", last_connected->valuestring);
                        }
                        
                        result = ESP_OK;
                    }
                }
                cJSON_Delete(response);
            }
        } else {
            ESP_LOGE(TAG, "❌ Failed to get device info. Status: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "❌ HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return result;
}


// =============================================================================
// Convenience Functions (Log levels)
// =============================================================================

static esp_err_t log_info(const char *message) {
    return bloomit_send_log("info", message);
}

static esp_err_t log_warning(const char *message) {
    return bloomit_send_log("warning", message);
}

static esp_err_t log_error(const char *message) {
    return bloomit_send_log("error", message);
}

static esp_err_t log_debug(const char *message) {
    return bloomit_send_log("debug", message);
}

// =============================================================================
// Sensor Functions (Example implementations)
// =============================================================================

static float s_simulated_temperature = 25.0f;
static float s_simulated_humidity = 60.0f;

/**
 * Read temperature from sensor
 * Replace with actual sensor reading code
 */
static float read_temperature(void) {
    // Simulate temperature variation
    s_simulated_temperature += ((esp_random() % 21) - 10) / 10.0f;
    if (s_simulated_temperature < 15.0f) s_simulated_temperature = 15.0f;
    if (s_simulated_temperature > 35.0f) s_simulated_temperature = 35.0f;
    return s_simulated_temperature;
}

/**
 * Read humidity from sensor
 * Replace with actual sensor reading code
 */
static float read_humidity(void) {
    // Simulate humidity variation
    s_simulated_humidity += ((esp_random() % 11) - 5) / 10.0f;
    if (s_simulated_humidity < 30.0f) s_simulated_humidity = 30.0f;
    if (s_simulated_humidity > 90.0f) s_simulated_humidity = 90.0f;
    return s_simulated_humidity;
}

/**
 * Send all sensor readings to Bloomit API
 */
static void send_all_sensor_readings(void) {
    float temp = read_temperature();
    float hum = read_humidity();

    ESP_LOGI(TAG, "\n🌡️ Current Readings:");
    ESP_LOGI(TAG, "   Temperature: %.2f °C", temp);
    ESP_LOGI(TAG, "   Humidity: %.2f %%", hum);

    char value_str[16];

    snprintf(value_str, sizeof(value_str), "%.2f", temp);
    bloomit_send_sensor_data("temperature", value_str);

    vTaskDelay(pdMS_TO_TICKS(500));  // Small delay between requests

    snprintf(value_str, sizeof(value_str), "%.2f", hum);
    bloomit_send_sensor_data("humidity", value_str);
}

// =============================================================================
// Main Application
// =============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║        Bloomit Device API - ESP-IDF Example               ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Load any stored credentials
    load_credentials();

    // Connect to WiFi
    ret = wifi_init_sta();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "⚠️ Could not connect to WiFi. Restarting in 10 seconds...");
        vTaskDelay(pdMS_TO_TICKS(10000));
        esp_restart();
    }

    // Wait a moment for network to stabilize
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Register device if not already registered
    ret = bloomit_register_device();
    if (ret != ESP_OK && strlen(s_device_token) == 0) {
        ESP_LOGE(TAG, "⚠️ Device registration failed!");
        ESP_LOGE(TAG, "   Make sure USER_TOKEN is set correctly.");
        ESP_LOGE(TAG, "   The device will retry in the main loop.");
    }

    // If registered, perform initial actions
    if (strlen(s_device_token) > 0) {
        log_info("Device started - ESP-IDF Example");
        
        // Get device info to verify registration
        bloomit_get_device_info();
    }

    ESP_LOGI(TAG, "\n════════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "Setup complete. Starting main loop...");
    ESP_LOGI(TAG, "════════════════════════════════════════════════════════════\n");

    // Main loop
    uint32_t last_sensor_time = 0;
    uint32_t last_heartbeat_time = 0;
    
    while (1) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // Try to register if not registered yet
        if (strlen(s_device_token) == 0) {
            bloomit_register_device();
            vTaskDelay(pdMS_TO_TICKS(30000));  // Wait 30 seconds before retrying
            continue;
        }

        // Send sensor readings every 30 seconds
        if (current_time - last_sensor_time > 30000 || last_sensor_time == 0) {
            last_sensor_time = current_time;
            send_all_sensor_readings();
        }

        // Send heartbeat log every 5 minutes
        if (current_time - last_heartbeat_time > 300000 || last_heartbeat_time == 0) {
            last_heartbeat_time = current_time;
            log_info("Heartbeat - Device running normally");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

