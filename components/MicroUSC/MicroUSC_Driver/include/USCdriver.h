#ifndef __USB_DRIVER_H
#define __USB_DRIVER_H

#include "esp_uart.h"
#include "memory_pool.h"
#include "atomic_sys_op.h"
#include "uscdef.h"

#ifdef __cplusplus
extern "C" {
#endif

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
extern Queue serial_data[DRIVER_MAX]; // DRIVER_MAX

extern usc_event_cb_t overdriver_action[OVERDRIVER_MAX]; // store all the actions of the overdrivers


esp_err_t init_priority_storage(void);

//void usc_driver_clean_data(usc_driver_t *driver);

// uint32_t usc_driver_get_data(const int i);

void queue_add(Queue *queue, const uint32_t data);

//uint32_t usc_driver_get_data(const int i);
/**
 * @brief Initialize the USB driver.
 *        Index is from 0 to DRIVER_MAX - 1 (0 to 1)
 * @param driver_setting Pointer to the USB configuration structure.
 * 
 * @return ESP_OK if port initialization and configuration is valid.
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

/**
 * @brief Deinitialize all USB drivers.
 * @param config Pointer to the USB configuration structure.
 * @param action Pointer to the event callback function.
 * @param i Index of the overdriver to be set.
 * @note The index should be between 0 and OVERDRIVER_MAX - 1.
 * @return ESP_OK if all arguments are valid
 */
esp_err_t usc_set_overdrive( usc_driver_t *overdriver, 
                             usc_event_cb_t action, 
                             const int i);

/**
 * @brief Set the overdrive driver.
 * @param driver Pointer to the serial input driver structure.
 * @param i Index of the overdriver to be set.
 * @note The index should be between 0 and OVERDRIVER_MAX - 1.
 * @return ESP_OK if the index is valid and the driver is set successfully.
 */
esp_err_t overdrive_usc_driver( usc_driver_t *driver,
                                const int i);

#ifdef __cplusplus
}
#endif

#endif // __USB_DRIVER_H