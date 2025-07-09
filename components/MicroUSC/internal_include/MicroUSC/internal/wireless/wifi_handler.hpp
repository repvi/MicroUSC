#pragma once

#include "MicroUSC/wireless/wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "string.h"
#include "esp_log.h"

class WifiMaintainer {
    public:
    void startWifi(wifi_config_t *wifi_config);
    void createWifiEventGroup();
    
    /* Create the default WiFi station interface */
    void createWifiInstance();

    void wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    private:

    EventGroupHandle_t wifi_event_group;
    wifi_config_t wifi_config;
    esp_netif_t *sta_netif; // Pointer to the default WiFi station interface

    uint8_t ssid[32];
    uint8_t password[32];
};