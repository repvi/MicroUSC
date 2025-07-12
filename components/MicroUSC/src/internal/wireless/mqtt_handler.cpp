#include "MicroUSC/internal/wireless/mqtt_handler.hpp"

#define TAG "[MQTT SERVICE]"

namespace MicroUSC 
{
    esp_err_t MqttMaintainer::start(const esp_mqtt_client_config_t &config)
    {
        /* Create binary semaphore for thread safety */
        this->mutex = xSemaphoreCreateBinaryStatic(&this->semaphore_buffer);
        if (this->mutex == NULL) {
            ESP_LOGE(TAG, "Could not initialize semaphore for MQTT service");
            return ESP_FAIL;
        }
        xSemaphoreGive(this->mutex);

        this->mqtt_device_map = hashmap_create(); /* Create a new hashmap for storing MQTT device subscriptions */
        if (check_device_name(NULL) == 0 && this->mqtt_device_map != NULL) {
            esp_err_t con = setupMqttService(this->config.buffer.size, this->config.buffer.out_size);
            if (con == ESP_OK) {
                ESP_LOGI(TAG, "MQTT service initialized successfully");
                setup_cjson_pool(); /* Initialize cJSON pool for JSON handling */
                return con; // Return success if all steps are successful
            }
            #ifdef MICROUSC_MQTT_DEBUG
            else {
                ESP_LOGE(TAG, "Failed to initialize MQTT service");
            }
            #endif
        }

        vSemaphoreDelete(this->mutex);
        this->mutex = NULL;
        return ESP_FAIL; // Return failure if any step fails
    }

    const char *MqttMaintainer::getDeviceName() const
    {
        return this->name; // Return the device name
    }

    const char *MqttMaintainer::getLastUpdated() const
    {
        return this->last_updated; // Return the last updated timestamp
    }

    const char *MqttMaintainer::getSensorType() const
    {
        return this->sensor_type; // Return the sensor type
    }

    esp_err_t MqttMaintainer::setupMqttService(int &buffer_size, int &out_size)
    {
        if (xSemaphoreTake(this->mutex, portMAX_DELAY) != pdFALSE) {
            /* Enforce minimum buffer sizes */
            if (buffer_size < 1024) {
                buffer_size = 1024; /* Set minimum buffer size */
            }

            if (out_size < 512) {
                out_size = 512; /* Set minimum output size */
            }
            /* Configure the MQTT client with the broker URI */
            /* Initialize the MQTT client */
            this->client = esp_mqtt_client_init(this->config);
            if (this->client != NULL) {
                /* Register the event handler for all MQTT events */
                esp_mqtt_client_register_event(this->client, ESP_EVENT_ANY_ID, mqtt_event_handler, this->client);
                /* Start the MQTT client (runs on core 0) */
                esp_err_t ca = esp_mqtt_client_start(this->client);
                if (ca == ESP_OK) {
                    ESP_LOGI(TAG, "Has been successfully been set");
                    xSemaphoreGive(this->mutex);
                    return ca;
                }
                #ifdef MICROUSC_MQTT_DEBUG
                else {
                    ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(ca));
                }
                #endif
            }
            #ifdef MICROUSC_MQTT_DEBUG
            else {
                ESP_LOGE(TAG, "Failed to initialize MQTT client");
            }
            #endif
            xSemaphoreGive(this->mutex);
        }
        #ifdef MICROUSC_MQTT_DEBUG
        else {
            ESP_LOGE(TAG, "Could not get semaphore for MQTT service setup");
        }
        #endif
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
}