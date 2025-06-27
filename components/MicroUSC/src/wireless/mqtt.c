#include "MicroUSC/internal/wireless/mqtt.h"
#include "MicroUSC/internal/wireless/wifi.h"
#include "MicroUSC/internal/wireless/parsing.h"
#include "MicroUSC/internal/hashmap.h"
#include "esp_system.h"
#include "esp_log.h"

#define TAG "[MQTT SERVICE]"

#define MQTT_TOPIC(x) x
#define CONNECTION_MQTT_SEND_INFO MQTT_TOPIC("device_info")
#define MQTT_DEVICE_CHANGE CONNECTION_MQTT_SEND_INFO
#define NO_NAME "No name"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
    bool connect;
    esp_mqtt_client_handle_t client;
    SemaphoreHandle_t mutex;
} mqtt_handler_t;

mqtt_handler_t mqtt_service;

HashMap mqtt_device_map; /* Stores subscription info */

char *device_name = NULL;
char *last_updated = ( __DATE__ );
char* sensor_type = "uart";

int send_to_mqtt_service(char *const topic, char const *const key, const char *const data, int len)
{
    cjson_pool_reset(); // Reset pool before building new JSON tree

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, key, data);
    
    char json_buffer[128];
    bool success = cJSON_PrintPreallocated(root, json_buffer, sizeof(json_buffer), 0);
    if (success) {
        if (len == 0) {
            len = strlen(json_buffer);
            if (len < 0) {
                ESP_LOGE(TAG, "Failed to create JSON payload");
                return -1;
            }
        }
        return esp_mqtt_client_publish(mqtt_service.client, topic, json_buffer, len, 1, 0);
    }
    else {
        return -2;
    }
}

static bool add_esp_mqtt_client_subscribe(
    const char *topic, 
    int qos,
    mqtt_event_data_action_t action
) {
    int id = esp_mqtt_client_subscribe(mqtt_service.client, topic, qos);
    if (id < 0) {
        ESP_LOGE(TAG, "Failed to subscribe to topic: %s", topic);
        return false; // Subscription failed
    }
    return hashmap_put(mqtt_device_map, topic, action);
}

static int send_connection_info(void) {
    if (device_name == NULL) {
        device_name = (char *)heap_caps_malloc(sizeof(NO_NAME), MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        strncpy(device_name, NO_NAME, sizeof(NO_NAME));
        if (device_name == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for device name");
            return -1; // Indicate failure
        }
    }
    #ifdef MICROUSC_MQTT_DEBUG
    int sent_status;

    sent_status = send_to_mqtt_service(CONNECTION_MQTT_SEND_INFO, "device_name", device_name, 0);
    if (sent_status < 0) {
        ESP_LOGE(TAG, "Failed to send connection info, status: %d", sent_status);
        return -1; // Indicate failure
    }

    sent_status = send_to_mqtt_service(CONNECTION_MQTT_SEND_INFO, "device_model", CONFIG_IDF_TARGET, 0);
    if (sent_status < 0) {
        ESP_LOGE(TAG, "Failed to send device model, status: %d", sent_status);
        return -1; // Indicate failure
    }

    sent_status = send_to_mqtt_service(CONNECTION_MQTT_SEND_INFO, "last_updated", last_updated, 0);
    if (sent_status < 0) {
        ESP_LOGE(TAG, "Failed to send last updated info, status: %d", sent_status);
        return -1; // Indicate failure
    }

    sent_status = send_to_mqtt_service(CONNECTION_MQTT_SEND_INFO, "sensor_type", sensor_type, 0);
    if (sent_status < 0) {
        ESP_LOGE(TAG, "Failed to send sensor type, status: %d", sent_status);
        return -1; // Indicate failure
    }
    #else
    send_to_mqtt_service(CONNECTION_MQTT_SEND_INFO, "device_name", device_name, 0);
    send_to_mqtt_service(CONNECTION_MQTT_SEND_INFO, "device_model", CONFIG_IDF_TARGET, 0);
    send_to_mqtt_service(CONNECTION_MQTT_SEND_INFO, "last_updated", last_updated, 0);
    send_to_mqtt_service(CONNECTION_MQTT_SEND_INFO, "sensor_type", sensor_type, 0);
    #endif

    return 1;
}

static void turnoff_led(esp_mqtt_event_handle_t event) 
{
    char *data = event->data;
    size_t data_len = event->data_len;
    char *led_status = get_cjson_string(event->data, "led_status");
    if (led_status == NULL) {
        ESP_LOGE(TAG, "LED status not found in data");
        return;
    }

    const size_t status_len = strlen(led_status);

    ESP_LOGI(TAG, "Received LED status: %.*s", status_len, led_status);

    if (strncmp(led_status, "on", min(status_len, 2)) == 0) {
        ESP_LOGI(TAG, "Turning on LED");
    } else if (strncmp(led_status, "off", min(status_len, 3)) == 0) {
        ESP_LOGI(TAG, "Turning off LED");
        // Example: Turn off LED by setting GPIO pin low
        // gpio_set_level(GPIO_NUM_2, 0); // Assuming GPIO 2 is used for LED
    } else {
        ESP_LOGW(TAG, "Unknown command for LED: %.*s", event->data_len, led_status);
    }
}

static void ota_handle(esp_mqtt_event_handle_t event) 
{
    char *data = event->data;
    size_t data_len = event->data_len;

    if (strncmp(data, MQTT_TOPIC("ota"), data_len) == 0) {
        if (strncmp(data, "update", data_len) == 0) {
            /* Example: Start OTA update task */
            // xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
        }
    }
}

static void handle_mqtt_data_recieved(esp_mqtt_event_handle_t event)
{
    char *key = event->topic;
    mqtt_event_data_action_t action = hashmap_get(mqtt_device_map, key);
    if (action) {
        action(event);
    }
    #ifdef MICROUSC_MQTT_DEBUG
    else {
        ESP_LOGW(TAG, "No action defined for topic: %.*s", event->topic_len, event->topic);
    }
    #endif
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
            if (!add_esp_mqtt_client_subscribe(MQTT_TOPIC(CONNECTION_MQTT_SEND_INFO), 0, turnoff_led)) {
                ESP_LOGE(TAG, "Failed to subscribe to topic: %s", MQTT_TOPIC(CONNECTION_MQTT_SEND_INFO));
                return;
            }
            if (!add_esp_mqtt_client_subscribe(MQTT_TOPIC("ota"), 0, ota_handle)) {
                ESP_LOGE(TAG, "Failed to subscribe to topic: %s", MQTT_TOPIC("ota"));
                return;
            }

            ESP_LOGI(TAG, "Successfully connected to MQTT broker");

            if (send_connection_info() < 0) {
                ESP_LOGI(TAG, "Connection info sent successfully");
                return;
            }
            break;
        case MQTT_EVENT_DISCONNECTED: {
            esp_err_t err = esp_mqtt_client_reconnect(mqtt_service.client);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "Failed to reconnect");
            }
        }
            break;
        case MQTT_EVENT_DATA:
            cJSON *root = check_cjson(event->data, event->data_len);
            if (root == NULL) {
                xSemaphoreGive(mqtt_service.mutex);
                return;
            }
            /* On data, print topic and data, and handle LED/OTA commands */
            #ifdef MICROUSC_MQTT_DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_DATA: Topic=%.*s, Data=%.*s", 
                     event->topic_len, event->topic, 
                     event->data_len, event->data);
            #endif
            handle_mqtt_data_recieved(event);
            break;
        default:
            /* Log other MQTT events */
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    xSemaphoreGive(mqtt_service.mutex);
}

esp_err_t init_mqtt(char *const url, size_t buffer_size, size_t out_size)
{
    /* Create binary semaphore for thread safety */
    vSemaphoreCreateBinary(mqtt_service.mutex);
    if (mqtt_service.mutex == NULL) {
        ESP_LOGE(TAG, "Could not intialize semaphore for mqtt service");
        return ESP_FAIL;
    }
    xSemaphoreGive(mqtt_service.mutex);

    /* Enforce minimum buffer sizes */
    if (buffer_size < 1024) {
        buffer_size = 1024; /* Set minimum buffer size */
    }

    if (out_size < 512) {
        out_size = 512; /* Set minimum output size */
    }

    setup_cjson_pool(); /* Initialize cJSON pool for JSON handling */

    mqtt_device_map = hashmap_create(); /* Create a new hashmap for storing MQTT device subscriptions */
    if (mqtt_device_map == NULL) {
        ESP_LOGE(TAG, "Failed to create hashmap for MQTT device map");
        return ESP_FAIL;
    }
    /* Configure the MQTT client with the broker URI */
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = url,
        .buffer.out_size = out_size, /* Set the output buffer size */
        .buffer.size = buffer_size, /* Set the input buffer size */
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

esp_err_t init_mqtt_with_device_info(const mqtt_device_info_t *device_info, char *const url, size_t buffer_size, size_t out_size)
{
    if (device_info->device_name == NULL) {
        
    }
    return init_mqtt(url, buffer_size, out_size);
}

void mqtt_service_deinit(void) 
{
    if (mqtt_service.client != NULL) {
        esp_mqtt_client_stop(mqtt_service.client);
        esp_mqtt_client_destroy(mqtt_service.client);
        mqtt_service.client = NULL;
    }

    if (mqtt_service.mutex != NULL) {
        vSemaphoreDelete(mqtt_service.mutex);
        mqtt_service.mutex = NULL;
    }

    if (mqtt_device_map != NULL) {
        hashmap_destroy(mqtt_device_map);
        mqtt_device_map = NULL;
    }

    if (device_name != NULL) {
        free(device_name);
        device_name = NULL;
    }
}