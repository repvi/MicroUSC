#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi in station mode
 * 
 * @param ssid     WiFi SSID string
 * @param password WiFi password string
 */
void wifi_init_sta(char *const ssid, char *const password);

/**
 * @brief Initialize WiFi using credentials from flash
 * 
 * @param ssid     WiFi SSID buffer
 * @param password WiFi password buffer
 * @param section  NVS section key
 */
void wifi_init_sta_get_password_on_flash(char *const ssid, char *const password, char *const section);

#ifdef __cplusplus
}
#endif