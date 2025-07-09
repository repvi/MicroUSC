#pragma once

#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WifiMaintainer WifiMaintainer;
typedef WifiMaintainer *WifiMaintainerHandler;

/**
 * @brief Initialize Wi-Fi in station mode
 * 
 * @param ssid     Null-terminated SSID string
 * @param password Null-terminated password string
 * @return         Handler to the Wi-Fi maintainer instance, or NULL on failure
 */
WifiMaintainerHandler wifi_init_sta(char *const ssid, char *const password);

/**
 * @brief Initialize WiFi using credentials from flash
 * 
 * @param ssid     WiFi SSID buffer
 * @param password WiFi password buffer
 * @param section  NVS section key
 */
void wifi_init_sta_get_password_on_flash(char *const ssid, char *const password, char *const section);

/**
 * @brief Check if station is connected to an AP
 * @return ESP_OK if connected, error code otherwise
 */
esp_err_t check_connection(void);

#ifdef __cplusplus
}
#endif