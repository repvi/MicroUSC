#include "MicroUSC/internal/wireless/wifi.h"
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

#define TAG "[WIFI]"

#define WIFI_ON   true
#define WIFI_OFF  false

#define NVS_WIFI_PASSWORD  "WiFl_$<Ss"
#define WIFI_CONNECTED_BIT BIT0

#define SSID_SIZE        sizeof(wifi_config.sta.ssid)
#define PASSWORD_SIZE    sizeof(wifi_config.sta.password)

char *hidden_ssid;
char *hidden_password;

bool wifi_sys = WIFI_OFF; /* do something with it */

EventGroupHandle_t wifi_event_group;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) 
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa((const ip4_addr_t*)&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(char *const ssid, char *const password)
{
    /* Create an event group for WiFi events */
    wifi_event_group = xEventGroupCreate();

    /* Initialize the TCP/IP network interface and event loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Create the default WiFi station interface */
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    /* Register WiFi and IP event handlers */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

    /* Configure WiFi with provided SSID and password */
    wifi_config_t wifi_config;
    memcpy(wifi_config.sta.ssid, ssid, SSID_SIZE);
    memcpy(wifi_config.sta.password, password, PASSWORD_SIZE);

    /* Set WiFi mode to station and apply configuration */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", ssid, password);

    wifi_sys = WIFI_ON;
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

void wifi_init_sta_get_passowrd_on_flash(char *const ssid, char *const password, char *const section) 
{
    /* Store and read WiFi credentials in NVS */
    set_password_nvs(ssid, password, section);

    /* Create an event group for WiFi events */
    wifi_event_group = xEventGroupCreate();

    /* Initialize the TCP/IP network interface and event loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Create the default WiFi station interface */
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    /* Register WiFi and IP event handlers */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

    /* Configure WiFi with provided SSID and password */
    wifi_config_t wifi_config;
    memcpy(wifi_config.sta.ssid, ssid, SSID_SIZE);
    memcpy(wifi_config.sta.password, password, PASSWORD_SIZE);

    /* Set WiFi mode to station and apply configuration */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", ssid, password);

    wifi_sys = WIFI_ON;
}