#ifndef __USB_DRIVER_H
#define __USB_DRIVER_H

#include "MicroUSC/system/memory_pool.h"
#include "MicroUSC/synced_driver/esp_uart.h"
#include "MicroUSC/synced_driver/atomic_sys_op.h"
#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STANDARD_UART_CONFIG { \
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE, /* should be defined by sdkconfig */ \
        .data_bits = UART_DATA_8_BITS, \
        .parity    = UART_PARITY_DISABLE, \
        .stop_bits = UART_STOP_BITS_1, \
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, \
    }

typedef void (*usc_event_cb_t)(void *);
typedef void (*usc_data_process_t)(void *);

typedef struct usc_config_t usc_config_t;

typedef struct usc_driver_t usc_driver_t;

typedef struct usc_task_manager_t usc_task_manager_t;
typedef usc_task_manager_t *usc_tasks_t;

// [leave alone]
extern QueueHandle_t uart_queue[DRIVER_MAX]; // Queue for UART data

// __attribute__((aligned(ESP32_ARCHITECTURE_ALIGNMENT_VAR)))
// atomic_uint_least32_t

typedef struct usc_bit_manip usc_bit_manip;

esp_err_t init_usc_bit_manip(usc_bit_manip *bit_manip);

esp_err_t init_configuration_storage(void);

//void usc_driver_clean_data(usc_driver_t *driver);

//uint32_t usc_driver_get_data(const int i);

/**
 * @brief Initialize a UART-based driver for the ESP32.
 * 
 * This function configures and installs a UART driver with the specified parameters,
 * assigns GPIO pins, and registers a data processing callback. It performs validation
 * of the UART configuration, port configuration, and callback function.
 * 
 * @param driver_name     Name of the driver for logging/identification (nullable).
 * @param uart_config     UART configuration (baud rate, data bits, parity, etc.).
 *                            Must be a valid `uart_config_t` struct.
 * @param port_config     UART port configuration (GPIO pins, buffer sizes, etc.).
 *                            Must include valid GPIO assignments and buffer sizes.
 * @param driver_process  Callback function to handle received serial data.
 *                            If NULL, initialization will fail.
 * 
 * @return
 * - ESP_OK: Driver initialized successfully.
 * - ESP_ERR_INVALID_ARG: Invalid `port_config` (e.g., invalid GPIO pins) or `driver_process` is NULL.
 * - ESP_FAIL: Invalid `uart_config` (e.g., unsupported baud rate) or UART driver installation failed.
 * 
 * @note
 * - The caller is responsible for ensuring GPIO pins are valid and not used by other peripherals.
 * - If `driver_name` is NULL, a default name will be used in logs.
 * - The UART driver must be uninstalled with `usc_driver_deinit()` when no longer needed.
 * 
 * @example
 * ```
 * uart_config_t uart_conf = {
 *     .baud_rate = 115200,
 *     .data_bits = UART_DATA_8_BITS,
 *     .parity = UART_PARITY_DISABLE,
 *     .stop_bits = UART_STOP_BITS_1,
 *     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
 * };
 * 
 * uart_port_config_t port_conf = {
 *     .port = UART_NUM_0,
 *     .tx_pin = GPIO_NUM_1,
 *     .rx_pin = GPIO_NUM_3,
 * };
 * 
 * esp_err_t ret = usc_driver_init("my_driver", uart_conf, port_conf, my_data_callback);
 * if (ret != ESP_OK) {
 *     // Handle error
 * }
 * ```
 */
esp_err_t usc_driver_init( const char *driver_name,
                           const uart_config_t uart_config, 
                           const uart_port_config_t port_config, 
                           usc_data_process_t driver_process);

/**
 * @brief Write data using the USB driver.
 *
 * @param config Pointer to the USB configuration structure.
 * @param data Pointer to the data to be written.
 * @param len Length of the data to be written.
 * 
 * @return ESP_OK if configuration is valid for the serial port.
 */
esp_err_t usc_driver_write( const usc_config_t *driver_setting, 
                            const char *data, 
                            const size_t len);

/**
 * @brief Request a password using the USB driver.
 * @param config Pointer to the USB configuration structure.
 * @return ESP_OK if sending serial data from the port works as intended
 */
esp_err_t usc_driver_request_password(usc_config_t *driver_setting);

/**
 * @brief Ping the USB driver.
 * @param config Pointer to the USB configuration structure.
 * @return
 *     - ESP_OK: Success
 * 
 *     - ESP_FAIL: Failed
 */
esp_err_t usc_driver_ping(usc_config_t *driver_setting);

//void uart_port_config_deinit(uart_port_config_t *uart_config);

#ifdef __cplusplus
}
#endif

#endif // __USB_DRIVER_H