#include "MicroUSC/internal/wireless/mqtt.h"
#include "esp_system.h"
#include "esp_log.h"

#define TAG "[MQTT SERVICE]"

typedef struct {
    bool connect;
    esp_mqtt_client_handle_t client;
    SemaphoreHandle_t mutex;
} mqtt_handler_t;

mqtt_handler_t mqtt_service;

// esp_mqtt_client_publish(client, "my/topic", "Hello from ESP32!", 0, 1, 0)

int send_to_mqtt_service(char *const section, char *const data)
{
    return esp_mqtt_client_publish(mqtt_service.client, section, data, 0, 1, 0);
}

void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) 
{
    if (xSemaphoreTake(mqtt_service.mutex, portMAX_DELAY) == pdFALSE) {
        ESP_LOGE(TAG, "Could not get semaphore");
        return;
    }

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            /* On connection, subscribe to sensor and LED topics */
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_subscribe(mqtt_service.client, HOME_DIR("sensor"), 0);
            esp_mqtt_client_subscribe(mqtt_service.client, HOME_DIR("led"), 0); // Subscribe to LED control topic
            break;
        case MQTT_EVENT_DISCONNECTED: {
            esp_err_t err = esp_mqtt_client_reconnect(mqtt_service.client);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "Failed to reconnect");
            }
        }
            break;
        case MQTT_EVENT_DATA:
            /* On data, print topic and data, and handle LED/OTA commands */
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            /* Example: LED control */
            if (strncmp(event->topic, HOME_DIR("led"), event->topic_len) == 0) {
                if (strncmp(event->data, "on", event->data_len) == 0) {
                    /* Do something */
                } else if (strncmp(event->data, "off", event->data_len) == 0) {
                    /* Do something */
                }
            } 
            /* Example: OTA update trigger */
            else if (strncmp(event->topic, HOME_DIR("ota"), event->topic_len) == 0) {
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

esp_err_t init_mqtt(char *const url)
{
    mqtt_service.mutex = vSemaphoreCreateBinary();
    if (mqtt_service.mutex == NULL) {
        ESP_LOGE(TAG, "Could not intialize semaphore for mqtt service");
        return ESP_FAIL;
    }
    xSemaphoreGive(mqtt_service.mutex);

    /* Configure the MQTT client with the broker URI */
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = url
    };

    /* Initialize the MQTT client */
    mqtt_service.client = esp_mqtt_client_init(&mqtt_cfg);

    /* Register the event handler for all MQTT events */
    esp_mqtt_client_register_event(mqtt_service.client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_service.client);

    /* Start the MQTT client (runs on core 0) */
    esp_err_t ca = esp_mqtt_client_start(mqtt_service.client);
    if (ca == ESP_OK) {
        ESP_LOGI(TAG, "Has been successfully been set");
        mqtt_service.connect = MQTT_ENABLED;
    }

    return ca;
}