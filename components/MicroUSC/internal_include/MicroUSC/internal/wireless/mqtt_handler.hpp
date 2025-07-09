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

class MqttMaintainer {
    public:
    esp_err_t setUp();
    private:

    esp_mqtt_client_config_t stored_info;
    esp_mqtt_client_handle_t client;

    HashMap mqtt_device_map;
    
    SemaphoreHandle_t mutex;

    char *name;
    char *last_updated;
    char *sensor_type;
};

int check_device_name(const char *new_name);