#include "MicroUSC/wireless/mqtt.h"
#include "MicroUSC/wireless/wifi.h"
#include "MicroUSC/wireless/parsing.h"
#include "MicroUSC/internal/hashmap.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"

#define TAG "[MQTT SERVICE]"

#define NO_NAME "No name"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
    bool active; // Indicates if the MQTT service is active
    esp_mqtt_client_handle_t client;
    SemaphoreHandle_t mutex;
} mqtt_handler_t;

typedef struct {
    esp_mqtt_event_handle_t event;
    cJSON *json; // JSON data parsed from event
} mqtt_data_package_t;

typedef void (*mqtt_event_data_action_t)(mqtt_data_package_t *package);

mqtt_handler_t mqtt_service = {.active = MQTT_DISABLED, .client = NULL, .mutex = NULL};

HashMap mqtt_device_map; /* Stores subscription info */

char *device_name = NULL;
char *last_updated = ( __DATE__ );
char* sensor_type = "uart";

const char *general_key[] = {
    "device_name",
    "device_model",
    "last_updated",
    "sensor_type"
};

static int check_device_name(const char *new_name) 
{
    int name_length;

    if (new_name == NULL && device_name == NULL) {
        name_length = sizeof(NO_NAME);
        // Allocate memory for the default device name
        device_name = (char *)heap_caps_malloc(name_length, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    } 
    else {
        if (device_name != NULL) {
            free(device_name); // Free previous name if it exists
        }
        name_length = strlen(new_name) + 1;
        device_name = (char *)heap_caps_malloc(name_length, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    }

    if (device_name == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for device name");
        return -1; // Indicate failure
    }
    strncpy(device_name, NO_NAME, name_length);

    return 0; // Indicate success
}

int send_to_mqtt_service_single(char *const topic, char const *const key, const char *const data)
{
    cjson_pool_reset(); // Reset pool before building new JSON tree

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, key, data);
    
    char json_buffer[128];
    bool success = cJSON_PrintPreallocated(root, json_buffer, sizeof(json_buffer), 0);
    if (success) {
        int len = strlen(json_buffer);
        if (len < 0) {
            ESP_LOGE(TAG, "Failed to create JSON payload");
            return -1;
        }
        return esp_mqtt_client_publish(mqtt_service.client, topic, json_buffer, len, 1, 0);
    }
    else {
        return -2;
    }
}

int send_to_mqtt_service_multiple(char *const topic, const char**key, const char**data, int len)
{
    cjson_pool_reset(); // Reset pool before building new JSON tree

    cJSON *root = cJSON_CreateObject();
    for (int i = 0; i < len; i++) {
        if (!key[i] || !data[i]) {
            ESP_LOGE(TAG, "Key or data is NULL at index %d", i);
            return -1; // Indicate failure
        }
        #ifdef MICROUSC_MQTT_DEBUG
        ESP_LOGI(TAG, "Adding key: %s with data: %s", key[i], data[i]);
        #endif
        cJSON_AddStringToObject(root, key[i], data[i]);
    }
    
    char json_buffer[256];
    bool success = cJSON_PrintPreallocated(root, json_buffer, sizeof(json_buffer), 0);
    if (success) {
        int json_string_size = strlen(json_buffer);
        if (json_string_size < 0) {
            ESP_LOGE(TAG, "Failed to create JSON payload");
            return -1;
        }
        return esp_mqtt_client_publish(mqtt_service.client, topic, json_buffer, json_string_size, 1, 0);
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
    char general_info[4][32];
    const char *info_ptrs[4];

    int device_name_length = min(strlen(device_name), 31);
    strncpy(general_info[0], device_name, device_name_length);
    general_info[0][device_name_length] = '\0'; // Ensure null-termination
    info_ptrs[0] = general_info[0];

    int device_model_length = min(strlen(CONFIG_IDF_TARGET), 31);
    strncpy(general_info[1], CONFIG_IDF_TARGET, device_model_length);
    general_info[1][device_model_length] = '\0'; // Ensure null-termination
    info_ptrs[1] = general_info[1];

    int last_updated_length = min(strlen(last_updated), 31);
    strncpy(general_info[2], last_updated, last_updated_length);
    general_info[2][last_updated_length] = '\0'; // Ensure null-termination
    info_ptrs[2] = general_info[2];

    int sensor_type_length = min(strlen(sensor_type), 31);
    strncpy(general_info[3], sensor_type, sensor_type_length);
    general_info[3][sensor_type_length] = '\0'; // Ensure null-termination
    info_ptrs[3] = general_info[3];

    return send_to_mqtt_service_multiple(
        CONNECTION_MQTT_SEND_INFO, 
        (const char **)general_key, 
        (const char **)info_ptrs, 
        4
    );
}

static void turnoff_led(mqtt_data_package_t *package) 
{
    esp_mqtt_event_handle_t event = package->event;
    (void)event; // Suppress unused variable warning
    
    // Extract LED status
    char *led_status = get_cjson_string(package->json, "led_status");
    if (led_status == NULL) {
        ESP_LOGE(TAG, "LED status not found in data");
        return;
    }
}

static void ota_handle(mqtt_data_package_t *package) 
{
    char *data = package->event->data;
    size_t data_len = package->event->data_len;

    (void)data; // Suppress unused variable warning
    (void)data_len; // Suppress unused variable warning

    /*
    if (strncmp(data, MQTT_TOPIC("ota"), data_len) == 0) {
        if (strncmp(data, "update", data_len) == 0) {
            // xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
        }
    }
    */
}

static void handle_mqtt_data_recieved(mqtt_data_package_t *package)
{
    char key[20];
    int key_len = min(package->event->topic_len, sizeof(key) - 1);
    strncpy(key, package->event->topic, key_len);
    key[key_len] = '\0';  // Ensure null-termination

    mqtt_event_data_action_t action = hashmap_get(mqtt_device_map, key);
    if (action) {
        action(package);
    }
    #ifdef MICROUSC_MQTT_DEBUG
    else {
        ESP_LOGW(TAG, "No action defined for topic: %.*s", package->event->topic_len, package->event->topic);
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
                goto exit; // Exit if subscription fails
            }

            if (!add_esp_mqtt_client_subscribe(MQTT_TOPIC("ota"), 0, ota_handle)) {
                ESP_LOGE(TAG, "Failed to subscribe to topic: %s", MQTT_TOPIC("ota"));
                goto exit; // Exit if subscription fails
            }

            ESP_LOGI(TAG, "Successfully connected to MQTT broker");

            if (send_connection_info() > 0) {
                ESP_LOGI(TAG, "Connection info sent successfully");
            }
            else {
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
            cJSON *root = check_cjson(event->data, event->data_len);
            if (root != NULL) {
                #ifdef MICROUSC_MQTT_DEBUG
                ESP_LOGI(TAG, "MQTT_EVENT_DATA: Topic=%.*s, Data=%.*s", 
                    event->topic_len, event->topic, 
                    event->data_len, event->data);
                #endif
                mqtt_data_package_t package = {
                    .event = event,
                    .json = root
                };
                handle_mqtt_data_recieved(&package);
            }
            break;
        default:
            /* Log other MQTT events */
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
exit:
    xSemaphoreGive(mqtt_service.mutex);
}

esp_err_t init_mqtt(esp_mqtt_client_config_t *mqtt_cfg)
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi not connected - MQTT start failed");
        return ESP_FAIL;
    }
    /* Create binary semaphore for thread safety */
    mqtt_service.mutex = xSemaphoreCreateBinary();
    if (mqtt_service.mutex == NULL) {
        ESP_LOGE(TAG, "Could not intialize semaphore for mqtt service");
        return ESP_FAIL;
    }
    xSemaphoreGive(mqtt_service.mutex);

    if (check_device_name(NULL) != 0) {
        goto clean_up;
    }

    /* Enforce minimum buffer sizes */
    if (mqtt_cfg->buffer.size < 1024) {
        mqtt_cfg->buffer.size = 1024; /* Set minimum buffer size */
    }

    if (mqtt_cfg->buffer.out_size < 512) {
        mqtt_cfg->buffer.out_size = 512; /* Set minimum output size */
    }

    setup_cjson_pool(); /* Initialize cJSON pool for JSON handling */

    mqtt_device_map = hashmap_create(); /* Create a new hashmap for storing MQTT device subscriptions */
    if (mqtt_device_map == NULL) {
        ESP_LOGE(TAG, "Failed to create hashmap for MQTT device map");
        goto clean_up;
    }
    /* Configure the MQTT client with the broker URI */

    /* Initialize the MQTT client */
    mqtt_service.client = esp_mqtt_client_init(mqtt_cfg);
    if (mqtt_service.client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        goto clean_up;
    }

    /* Register the event handler for all MQTT events */
    esp_mqtt_client_register_event(mqtt_service.client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_service.client);

    /* Start the MQTT client (runs on core 0) */
    esp_err_t ca = esp_mqtt_client_start(mqtt_service.client);
    if (ca != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(ca));
        goto clean_up;
    }

    ESP_LOGI(TAG, "Has been successfully been set");
    mqtt_service.active = MQTT_ENABLED;
    return ca;

clean_up:
    vSemaphoreDelete(mqtt_service.mutex);
    mqtt_service.mutex = NULL;
    return ESP_FAIL; // Return failure if any step fails
}

esp_err_t init_mqtt_with_device_info(esp_mqtt_client_config_t *mqtt_cfg, const mqtt_device_info_t *device_info)
{
    if (check_device_name(device_info->device_name) != 0) {
        return ESP_FAIL; // Device name is not set
    }
    return init_mqtt(mqtt_cfg);
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
        //hashmap_remove(mqtt_device_map);
        mqtt_device_map = NULL;
    }

    if (device_name != NULL) {
        free(device_name);
        device_name = NULL;
    }

    mqtt_service.active = false; // Mark the MQTT service as inactive
    ESP_LOGI(TAG, "MQTT service deinitialized");
}