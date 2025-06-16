#include "MicroUSC/internal/wireless/mqtt.h"
#include "esp_system.h"
#include "esp_log.h"

#define TAG "[MQTT SERVICE]"

bool mqtt_connect = MQTT_DISABLED;

esp_mqtt_client_handle_t client;
// esp_mqtt_client_publish(client, "my/topic", "Hello from ESP32!", 0, 1, 0)

void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) 
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            /* On connection, subscribe to sensor and LED topics */
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_subscribe(client, "home/sensor", 0);
            esp_mqtt_client_subscribe(client, "home/led", 0); // Subscribe to LED control topic
            break;
        case MQTT_EVENT_DATA:
            /* On data, print topic and data, and handle LED/OTA commands */
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            /* Example: LED control */
            if (strncmp(event->topic, "home/led", event->topic_len) == 0) {
                if (strncmp(event->data, "on", event->data_len) == 0) {
                    /* Example: Turn on LED */
                    // gpio_set_level(LED_GPIO_PIN, ON);
                } else if (strncmp(event->data, "off", event->data_len) == 0) {
                    /* Example: Turn off LED */
                    // gpio_set_level(LED_GPIO_PIN, OFF);
                }
            } 
            /* Example: OTA update trigger */
            else if (strncmp(event->topic, "home/ota", event->topic_len) == 0) {
                if (strncmp(event->data, "update", event->data_len) == 0) {
                    /* Example: Start OTA update task */
                    // xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
                }
            }
            break;
        default:
            /* Log other MQTT events */
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void init_mqtt(char *const url)
{
    /* Configure the MQTT client with the broker URI */
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = url
    };

    /* Initialize the MQTT client */
    client = esp_mqtt_client_init(&mqtt_cfg);

    /* Register the event handler for all MQTT events */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

    /* Start the MQTT client (runs on core 0) */
    esp_err_t err = esp_mqtt_client_start(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Has been successfully been set");
        mqtt_connect = MQTT_ENABLED;
    }
}