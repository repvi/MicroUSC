#include "MicroUSC/internal/wireless/wifi_handler.hpp"

#define TAG "[WIFI SERVICE]"

void  WifiMaintainer::startWifi(wifi_config_t *wifi_config) 
{
    const size_t SSID_SIZE = sizeof(this->wifi_config.sta.ssid);
    const size_t PASSWORD_SIZE = sizeof(this->wifi_config.sta.password);
    
    this->wifi_config = *wifi_config;

    /* Configure WiFi with provided SSID and password */
    strncpy(reinterpret_cast<char *>(this->wifi_config.sta.ssid), reinterpret_cast<char *>(this->wifi_config.sta.ssid), SSID_SIZE);
    this->wifi_config.sta.ssid[sizeof(this->wifi_config.sta.ssid) - 1] = '\0';

    strncpy(reinterpret_cast<char *>(this->wifi_config.sta.password), reinterpret_cast<char *>(this->wifi_config.sta.password), PASSWORD_SIZE);
    this->wifi_config.sta.password[sizeof(this->wifi_config.sta.password) - 1] = '\0';

    /* Set WiFi mode to station and apply configuration */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &this->wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void WifiMaintainer::createWifiEventGroup()
{
    this->wifi_event_group = xEventGroupCreate();
}

void WifiMaintainer::createWifiInstance()
{
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
}

void WifiMaintainer::wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    const EventBits_t WIFI_CONNECTED_BIT = BIT0;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(this->wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa((const ip4_addr_t*)&event->ip_info.ip));
        xEventGroupSetBits(this->wifi_event_group, WIFI_CONNECTED_BIT);
    }
}