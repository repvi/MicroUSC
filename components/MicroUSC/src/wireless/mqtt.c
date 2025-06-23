#include "MicroUSC/internal/wireless/mqtt.h"
#include "MicroUSC/internal/wireless/wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "cJSON.h"

#define TAG "[MQTT SERVICE]"

#define CONNECTION_MQTT_SEND_INFO  "connection"
#define NO_NAME "No name"

typedef struct {
    bool connect;
    esp_mqtt_client_handle_t client;
    SemaphoreHandle_t mutex;
} mqtt_handler_t;

mqtt_handler_t mqtt_service;

#define CJSON_POOL_SIZE 512

#define MQTT_TOPIC(x) "prot/"x

char *const device_name = NULL;
char *last_updated = ( __DATE__ );
char* sensor_type = "uart";

uint8_t cjson_pool[CJSON_POOL_SIZE];
size_t cjson_pool_offset = 0;

void *my_pool_malloc(size_t sz) {
    if (cjson_pool_offset + sz > CJSON_POOL_SIZE) {
        return NULL; // Out of memory!
    }
    void *ptr = &cjson_pool[cjson_pool_offset];
    cjson_pool_offset += sz;
    return ptr;
}

void my_pool_free(void *ptr) {
    // No-op: can't free individual blocks in bump allocator
    (void)ptr;
}

void cjson_pool_reset(void) {
    cjson_pool_offset = 0;
}

void setup_cjson_pool(void) {
    cJSON_Hooks hooks = {
        .malloc_fn = my_pool_malloc,
        .free_fn = my_pool_free
    };
    cJSON_InitHooks(&hooks);
}

int send_to_mqtt_service(char *const section, char *const data)
{
    cjson_pool_reset(); // Reset pool before building new JSON tree

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, section, data);
    
    char json_buffer[128];
    bool success = cJSON_PrintPreallocated(root, json_buffer, sizeof(json_buffer), 0);
    if (success) {
        return esp_mqtt_client_publish(mqtt_service.client, section, json_buffer, 0, 1, 0);
    }
    else {
        return -2;
    }
}

static int send_connection_info(void) {
    cjson_pool_reset(); /* Reset pool before building new JSON tree */
    cJSON *root = cJSON_CreateObject();
    if (device_name == NULL) {
        device_name = (char *)heap_caps_malloc(sizeof(NO_NAME), MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        strncpy(device_name, NO_NAME, sizeof(NO_NAME));
    }

    cJSON_AddStringToObject(root, "device_name", device_name);
    cJSON_AddStringToObject(root, "device_model", CONFIG_IDF_TARGET);
    cJSON_AddStringToObject(root, "last_updated", last_updated);
    //cJSON_AddStringToObject(root, "status", "connected");
    cJSON_AddStringToObject(root, "sensor_type", sensor_type);
    
    char json_buffer[128];
    bool success = cJSON_PrintPreallocated(root, json_buffer, sizeof(json_buffer), 0);
    if (success) {
        return esp_mqtt_client_publish(mqtt_service.client, MQTT_PROTOCOL_TYPE("connection"), json_buffer, 0, 1, 0);
    }
    else {
        return -2;
    }
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
            esp_mqtt_client_subscribe(mqtt_service.client, MQTT_TOPIC("sys"), 0);
            esp_mqtt_client_subscribe(mqtt_service.client, MQTT_TOPIC("ota"), 0);
            
            int sta = send_connection_info();
            if (sta == -2) {
                ESP_LOGE(TAG, "Failed to send connection info");
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
            /* On data, print topic and data, and handle LED/OTA commands */
            // ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            /* Example: LED control */
            if (strncmp(event->topic, MQTT_TOPIC("sys"), event->topic_len) == 0) {
                if (strncmp(event->data, "on", event->data_len) == 0) {
                    /* Do something */
                } else if (strncmp(event->data, "off", event->data_len) == 0) {
                    /* Do something */
                }
            } 
            /* Example: OTA update trigger */
            else if (strncmp(event->topic, MQTT_TOPIC("ota"), event->topic_len) == 0) {
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
    xSemaphoreGive(mqtt_service.mutex);
}

esp_err_t init_mqtt(char *const url)
{
    vSemaphoreCreateBinary(mqtt_service.mutex);
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