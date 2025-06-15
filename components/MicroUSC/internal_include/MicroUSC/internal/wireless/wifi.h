#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Initialize WiFi in station mode with provided SSID and password.
 *
 * This function sets up the ESP32/ESP8266 WiFi interface in station (client) mode.
 * It creates the event group, initializes the network interface and event loop,
 * registers WiFi and IP event handlers, configures the WiFi with the provided SSID and password,
 * and starts the WiFi driver. It then attempts to connect to the specified access point.
 *
 * @param ssid     Pointer to a null-terminated string containing the WiFi SSID.
 * @param password Pointer to a null-terminated string containing the WiFi password.
 */
void wifi_init_sta(char *const ssid, char *const password);

/**
 * @brief Initialize WiFi in station mode using credentials stored in flash.
 *
 * This function initializes NVS, stores the provided SSID and password in NVS,
 * then reads them back into hidden buffers. It then proceeds to initialize WiFi
 * in station mode using the provided (or just-read) SSID and password, following
 * the same steps as wifi_init_sta.
 *
 * @param ssid     Pointer to a buffer containing the WiFi SSID (input/output).
 * @param password Pointer to a buffer containing the WiFi password (input/output).
 * @param section  Pointer to a null-terminated string specifying the NVS section/key to use.
 */
void wifi_init_sta_get_passowrd_on_flash(char *const ssid, char *const password, char *const section);

#ifdef __cplusplus
}
#endif