#pragma once

#include "MicroUSC/wireless/mqtt.h"
#include "MicroUSC/wireless/ota.h"
#include "MicroUSC/wireless/wifi.h"
#include "MicroUSC/wireless/parsing.h"
#include "MicroUSC/internal/hashmap.h"
#include "stdatomic.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/atomic.h"

struct mqtt_data_package_t {
    esp_mqtt_event_handle_t event;
    cJSON *json; // JSON data parsed from event
};

typedef void (*mqtt_event_data_action_t)(mqtt_data_package_t *package);

namespace MicroUSC {

    class MqttMaintainer {
        public:

        static const int STRING_SIZE = 32; // Size for string buffers
        
        esp_err_t start(const esp_mqtt_client_config_t &config);
    
        const char *getDeviceName() const;
        const char *getLastUpdated() const;
        const char *getSensorType() const;
        
        private:
    
        esp_err_t setupMqttService(int &buffer_size, int &out_size);
    
        esp_mqtt_client_config_t config;
        esp_mqtt_client_handle_t client;

        HashMap mqtt_device_map;
    
        StaticSemaphore_t semaphore_buffer;
        SemaphoreHandle_t mutex;

        char name[STRING_SIZE];
        char last_updated[STRING_SIZE];
        char sensor_type[STRING_SIZE];
    };

    int check_device_name(const char *new_name);
}

void handle_mqtt_data_received(mqtt_data_package_t *package);