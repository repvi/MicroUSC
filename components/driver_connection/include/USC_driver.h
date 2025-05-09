#ifndef __USB_DRIVER_H
#define __USB_DRIVER_H

#include "esp_uart.h"
#include "memory_pool.h"
#include "atomic_sys_op.h"

#ifdef __cplusplus
extern "C" {
#endif

struct usc_stored_config_t;
struct usc_task_manager_t;

#define literate_bytes(var, type, len) \
    for ( \
        type* begin = (var), \
        *end = (var) + (len); \
        (begin) < (end); (begin)++ \
    ) // used for the byte loop

#define define_iteration(var, type, name, len) \
    for ( \
        type* (name) = (var), \
        *end = (var) + (len); \
        (name) < (end); (name)++ \
    ) // used for the byte loop

#define define_iteration_with_semaphore(var, type, name, len) \
    bool (hasSemaphore) = xSemaphoreTake(var->sync_signal, SEMAPHORE_DELAY);\
    for ( \
        type* (name) = (var), \
        *end = (var) + (len); \
        (name) < (end); \
        (name) += hasSemaphore, \
        (hasSemaphore) = xSemaphoreTake(name->sync_signal, SEMAPHORE_DELAY) \
    ) /* used for the byte loop */ \

#define cycle_usc_tasks() { \
        for ( \
            usc_driver_t *driver = (driver_task_manager), \
            *end = (driver_task_manager) + (DRIVER_MAX); \
            usc_task < end; usc_task++ \
        ) \
    } // used for the driver loop

#define cycle_drivers() define_iteration_with_semaphore(drivers, usc_driver_t, driver, DRIVER_MAX) // used for the driver loop
#define cycle_overdrivers() define_iteration_with_semaphore(overdrivers, usc_driver_t, overdriver, OVERDRIVER_MAX) // used for the overdriver loop

typedef void (*usc_event_cb_t)(void *);
typedef void (*usc_data_process_t)(void *);
typedef char driver_name_t[20];
typedef char serial_key_t[10];

#define DRIVER_NAME_SIZE              ( sizeof( driver_name_t ) )

/**
 * @brief Status of driver connection
 */
typedef enum {
    NOT_CONNECTED,         ///< Device is not connected.
    CONNECTED,             ///< Device is connected.
    DISCONNECTED,          ///< Device is disconnected.
    ERROR,                 ///< An error occurred.
    DATA_RECEIVED,         ///< Data has been received.
    DATA_SENT,             ///< Data has been sent.
    DATA_SEND_ERROR,       ///< Error sending data.
    DATA_RECEIVE_ERROR,    ///< Error receiving data.
    DATA_SEND_TIMEOUT,     ///< Timeout sending data.
    DATA_RECEIVE_TIMEOUT,  ///< Timeout receiving data.
    DATA_SEND_COMPLETE,    ///< Data send operation is complete.
    DATA_RECEIVE_COMPLETE, ///< Data receive operation is complete.
    TIME_OUT,              ///< Operation timed out.
} usc_status_t;

/**
 * @brief Structure for USB configuration.
 */
typedef struct {
    uart_port_config_t uart_config; ///< UART configuration structure.
    Queue data; // UART_NUM_MAX is used as the size of the stored data in the port
    driver_name_t driver_name; ///< Name of the driver.
    usc_status_t status; ///< Current status of the USB connection.
    uint32_t baud_rate; ///< Baud rate of the UART (not implemented yet).
    bool has_access; ///< Flag indicating if access is granted.
} usc_config_t;

typedef struct {
    TaskHandle_t task_handle; // Task handle for the driver task (uart controller)
    TaskHandle_t action_handle; // Task handle for the action task (reader and writer)
    bool active;
} usc_task_manager_t;

typedef struct {
    usc_config_t driver_setting;                    ///< Array of USB configurations
    usc_task_manager_t driver_tasks;  ///< Task manager for the drivers
    SemaphoreHandle_t sync_signal;
} usc_driver_t;

extern usc_driver_t drivers[DRIVER_MAX]; // System drivers for the USC driver
extern QueueHandle_t uart_queue[DRIVER_MAX]; // Queue for UART data

// __attribute__((aligned(ESP32_ARCHITECTURE_ALIGNMENT_VAR)))
extern atomic_uint_least32_t serial_data[DRIVER_MAX] ESP32_ALIGNMENT;

extern usc_driver_t overdrivers[OVERDRIVER_MAX];
extern usc_event_cb_t overdriver_action[OVERDRIVER_MAX]; // store all the actions of the overdrivers

uint32_t usc_driver_get_data(const int i);

void queue_add(Queue *queue, const uint32_t data);

//uint32_t usc_driver_get_data(const int i);
/**
 * @brief Initialize the USB driver.
 *        Index is from 0 to DRIVER_MAX - 1 (0 to 1)
 * @param config Pointer to the USB configuration structure.
 * @param event_cb Pointer to the event callback function.
 * 
 * @return ESP_OK if port initialization and configuration is valid.
 */
esp_err_t usc_driver_init( usc_config_t *driver_setting, 
                           const uart_config_t uart_config, 
                           const uart_port_config_t port_config, 
                           usc_data_process_t driver_process, 
                           const UBaseType_t i);

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

void uart_port_config_deinit(uart_port_config_t *uart_config);

/**
 * @brief Deinitialize all USB drivers.
 * @param config Pointer to the USB configuration structure.
 * @param action Pointer to the event callback function.
 * @param i Index of the overdriver to be set.
 * @note The index should be between 0 and OVERDRIVER_MAX - 1.
 * @return ESP_OK if all arguments are valid
 */
esp_err_t usc_set_overdrive( const usc_config_t *driver_setting, 
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