#include "MicroUSC/internal/wireless/mqtt_handler.hpp"

esp_err_t MqttMaintainer::setUp() 
{
    /* Create binary semaphore for thread safety */
    this->mutex = xSemaphoreCreateBinary();
    if (this->mutex == NULL) {
        ESP_LOGE(TAG, "Could not initialize semaphore for MQTT service");
        return ESP_FAIL;
    }
    xSemaphoreGive(this->mutex);

    this->mqtt_device_map = hashmap_create(); /* Create a new hashmap for storing MQTT device subscriptions */
    if (check_device_name(NULL) == 0 && mqtt_device_map != NULL) {        
        esp_err_t con = setup_mqtt_service(mqtt_service.mutex, &mqtt_cfg->buffer.size, &mqtt_cfg->buffer.out_size);
        if (con == ESP_OK) {
            ESP_LOGI(TAG, "MQTT service initialized successfully");
            setup_cjson_pool(); /* Initialize cJSON pool for JSON handling */
            set_mqtt_service_info(); /* Set MQTT service information */
            return con; // Return success if all steps are successful
        }
        #ifdef MICROUSC_MQTT_DEBUG
        else {
            ESP_LOGE(TAG, "Failed to initialize MQTT service");
        }
        #endif
    }

    vSemaphoreDelete(mqtt_service.mutex);
    mqtt_service.mutex = NULL;
    return ESP_FAIL; // Return failure if any step fails
}


int check_device_name(const char *new_name)
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