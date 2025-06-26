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

#define HOME_DIR(label) "home/"label

typedef void (*mqtt_event_data_action_t)(esp_mqtt_event_handle_t event);

/**
 * @brief Send data to MQTT broker as JSON
 *
 * Creates and sends a JSON message to the MQTT broker with the format:
 * {
 *    "<section>": "<data>"
 * }
 *
 * @param section    Topic section/key for the JSON object
 * @param data      Data string to be sent as value
 *
 * @return          Message ID on success (>0)
 *                  -2 if JSON creation/printing fails
 *                  -1 if MQTT publish fails
 *
 * @note Uses a static 128-byte buffer for JSON string
 * @note Resets JSON memory pool before operation
 * 
 * Example:
 * ```
 * // Sends: {"temperature": "25.5"}
 * send_to_mqtt_service("temperature", "25.5");
 * ```
 */
int send_to_mqtt_service(char *const topic, char const *const key, const char *const data);

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
esp_err_t init_mqtt(char *const url, size_t buffer_size, size_t out_size);