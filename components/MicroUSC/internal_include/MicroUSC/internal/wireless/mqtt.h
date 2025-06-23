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

int send_to_mqtt_service(char *const section, char *const data);

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

esp_err_t init_mqtt(char *const url);