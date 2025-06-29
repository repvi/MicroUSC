#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_http_client.h"
#include "driver/gpio.h"

#define MQTT_ENABLED   true
#define MQTT_DISABLED  false

#define MQTT_TOPIC(x) x
#define CONNECTION_MQTT_SEND_INFO MQTT_TOPIC("device_info")
#define MQTT_DEVICE_CHANGE CONNECTION_MQTT_SEND_INFO

typedef struct {
    char *device_name;
    char *last_updated;
    char *sensor_type;
} mqtt_device_info_t;

/**
 * @brief Send single key-value pair as JSON to MQTT topic
 * @param topic MQTT topic
 * @param key   JSON key
 * @param data  JSON value
 * @return >0 on success, -1 on JSON fail, -2 if buffer too small
 */
int send_to_mqtt_service_single(char *const topic, char const *const key, const char *const data);

/**
 * @brief Send multiple key-value pairs as JSON to MQTT topic
 * @param topic MQTT topic
 * @param key   Array of JSON keys
 * @param data  Array of JSON values
 * @param len   Number of key-value pairs
 * @return >0 on success, -1 on JSON fail, -2 if buffer too small
 */
int send_to_mqtt_service_multiple(char *const topic, const char**key, const char**data, int len);

/**
 * @brief MQTT event handler for ESP-IDF MQTT client.
 *
 * Handles various MQTT events such as connection, data reception, and others.
 * - On connection, subscribes to "home/sensor" and "home/led" topics.
 * - On data reception, prints the topic and data, and performs actions based on topic/data:
 *     - For "home/led": (example) would toggle an LED based on "on"/"off" message.
 *     - For "home/ota": (example) would trigger OTA update if "update" message is received.
 *
 * @param handler_args  User-defined handler arguments (unused).
 * @param base          Event base (unused).
 * @param event_id      Event ID (MQTT event type).
 * @param event_data    Pointer to event-specific data (esp_mqtt_event_handle_t).
 */
void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);

/**
 * @brief Initialize MQTT client service with specified parameters
 *
 * @param url          MQTT broker URL
 * @param buffer_size  Size for receive buffer (minimum 1024 bytes)
 * @param out_size     Size for output buffer (minimum 512 bytes)
 *
 * @return ESP_OK if initialization successful
 *         ESP_FAIL if semaphore creation fails
 *         Other ESP errors from MQTT client operations
 *
 * Function flow:
 * 1. Creates and initializes mutex for thread safety
 * 2. Adjusts buffer sizes to minimums if needed
 * 3. Configures and starts MQTT client
 */
esp_err_t init_mqtt(esp_mqtt_client_config_t *mqtt_cfg);

/**
 * @brief Initialize MQTT client with device information
 *
 * This function initializes the MQTT client using the provided device information.
 * It sets up the MQTT client configuration and starts the client.
 *
 * @param device_info  Pointer to a structure containing device information
 * @param url          MQTT broker URL
 * @param buffer_size  Size for receive buffer (minimum 1024 bytes)
 * @param out_size     Size for output buffer (minimum 512 bytes)
 *
 * @return ESP_OK if initialization successful
 *         ESP_FAIL if semaphore creation fails or other errors occur
 */
esp_err_t init_mqtt_with_device_info(esp_mqtt_client_config_t *mqtt_cfg, const mqtt_device_info_t *device_info);

/**
 * @brief Deinitialize MQTT service and clean up resources
 *
 * Frees allocated memory, destroys mutex, and stops MQTT client.
 * Should be called when MQTT service is no longer needed.
 */
void mqtt_service_deinit(void);