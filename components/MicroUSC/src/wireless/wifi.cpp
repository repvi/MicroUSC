#include "MicroUSC/internal/wireless/wifi_handler.hpp"
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
#include "stdatomic.h"
#define TAG "[WIFI]"

#define WIFI_ON   true
#define WIFI_OFF  false

#define NVS_WIFI_PASSWORD  "WiFl_$<Ss"

void wifiEventHandlerWrapper(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) 
{
    WifiMaintainerHandler handler = reinterpret_cast<WifiMaintainerHandler>(arg);
    handler->wifiEventHandler(arg, event_base, event_id, event_data);
}

static WifiMaintainerHandler wifi_init_sta_config(char *const ssid, char *const password)
{
    WifiMaintainerHandler handler = reinterpret_cast<WifiMaintainerHandler>(heap_caps_malloc(sizeof(WifiMaintainer), MALLOC_CAP_8BIT | MALLOC_CAP_DMA));
    if (handler != NULL) {
         /* Create an event group for WiFi events */
        handler->createWifiEventGroup();

        ESP_ERROR_CHECK(nvs_flash_init());
        /* Initialize the TCP/IP network interface and event loop */
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        handler->createWifiInstance();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        /* Register WiFi and IP event handlers */
        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEventHandlerWrapper, handler, &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifiEventHandlerWrapper, handler, &instance_got_ip));
    
        wifi_config_t wifi_config;

        strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy(reinterpret_cast<char *>(wifi_config.sta.password), password, sizeof(wifi_config.sta.password) - 1);
        handler->startWifi(&wifi_config);
    
        ESP_LOGI(TAG, "wifi_init_sta finished.");
        #ifdef SYSTEM_WIFI_DEBUG
        ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", ssid, password);
        #endif
    }
    return handler;
}

extern "C" WifiMaintainerHandler wifi_init_sta(char *const ssid, char *const password)
{
    return wifi_init_sta_config(ssid, password);
}

static void init_nvs(void) 
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    // ESP_ERROR_CHECK(err);
}

static void store_wifi_information_nvs(const char* str, const char *section) 
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open(section, NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_str(handle, "wifi_pass", str));
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

static void read_wifi_information_nvs(char* out_buffer, const char *section, size_t buffer_size) 
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open(section, NVS_READONLY, &handle));
    size_t required_size = buffer_size;
    esp_err_t err = nvs_get_str(handle, "wifi_pass", out_buffer, &required_size);
    if (err == ESP_OK) {
        // Password is now in out_buffer
    }
    nvs_close(handle);
}

static void set_password_nvs(char *const ssid, char *const password, char *const section) 
{
    init_nvs();
    store_wifi_information_nvs(ssid, section);
    store_wifi_information_nvs(password, section);
    //read_wifi_information_nvs(hidden_ssid, section, strlen(ssid)):
    //read_wifi_information_nvs(hidden_password, section, strlen(password));
}

extern "C" void wifi_init_sta_get_password_on_flash(char *const ssid, char *const password, char *const section) 
{
    /* Store and read WiFi credentials in NVS */
    set_password_nvs(ssid, password, section);
    wifi_init_sta_config(ssid, password);
}

extern "C" esp_err_t check_connection(void) 
{
    wifi_ap_record_t ap_info;
    return esp_wifi_sta_get_ap_info(&ap_info);
}