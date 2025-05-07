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

#define cycle_drivers() \
    for ( \
        usc_stored_config_t *driver = (drivers), \
        *end = (drivers) + (DRIVER_MAX); \
        driver < end; driver++ \
    ) // used for the driver loop

#define cycle_overdrivers() \
    for ( \
        usc_stored_config_t *overdriver = (overdrivers), \
        *end = overdrivers + (OVERDRIVER_MAX); \
        overdriver < end; overdriver++ \
    ) // used for the overdriver loop

#define cycle_usc_tasks() \
    for ( \
        usc_task_manager_t *usc_task = (driver_task_manager), \
        *end = (driver_task_manager) + (DRIVER_MAX); \
        usc_task < end; usc_task++ \
    ) // used for the driver loop

typedef struct stored_uart_data_t {
    uint32_t data; // change in the future
    struct stored_uart_data_t *next;
} stored_uart_data_t;

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
    driver_name_t driver_name; ///< Name of the driver.
    Queue data; // UART_NUM_MAX is used as the size of the stored data in the port
    usc_status_t status; ///< Current status of the USB connection.
    uint32_t baud_rate; ///< Baud rate of the UART (not implemented yet).
    bool has_access; ///< Flag indicating if access is granted.
} usc_config_t;

/**
 * @brief Structure for serial input drivers.
 */
typedef struct {
    usc_config_t config; ///< Array of USB configurations
    usc_data_process_t driver_action; ///< Action callback for the overdrive driver.
} serial_input_driver_t;

typedef struct {
    TaskHandle_t task_handle; // Task handle for the driver task
    TaskHandle_t action_handle; // Task handle for the action task
    bool active;
} usc_task_manager_t;

// save a lot of memory
typedef struct {
    usc_config_t *config;                    ///< Array of USB configurations
    portMUX_TYPE critical_lock;              ///< Mutex lock for the queue
    usc_data_process_t driver_action;        ///< Action callback for the overdrive driver
} usc_stored_config_t;

extern usc_stored_config_t drivers[DRIVER_MAX];

// __attribute__((aligned(ESP32_ARCHITECTURE_ALIGNMENT_VAR)))
extern atomic_uint_least32_t serial_data[DRIVER_MAX] ESP32_ALIGNMENT;  // Indexed access [1][3]

extern usc_task_manager_t driver_task_manager[DRIVER_MAX]; // Task manager for the drivers

extern usc_stored_config_t overdrivers[OVERDRIVER_MAX];

/**
 * @brief Type definition for overdriver size.
 */
typedef unsigned int overdriver_size_t;

uint32_t usb_driver_get_data(const int i);

extern esp_err_t usc_driver_deinit_all(void); // used in the kernel for the driver

extern esp_err_t usc_overdriver_deinit_all(void); // used in the kernel for the overdriver

void queue_add(Queue *queue, const uint32_t data);

uint32_t usb_driver_get_data(const int i);
/**
 * @brief Initialize the USB driver.
 *        Index is from 0 to DRIVER_MAX - 1 (0 to 1)
 * @param config Pointer to the USB configuration structure.
 * @param event_cb Pointer to the event callback function.
 * 
 * @return ESP_OK if port initialization and configuration is valid.
 */
esp_err_t usc_driver_init( usc_config_t *config, 
                           const uart_config_t uart_config, 
                           const uart_port_config_t port_config, 
                           usc_data_process_t driver_process, 
                           const UBaseType_t i, 
                           QueueHandle_t uart_queue);

/**
 * @brief Write data using the USB driver.
 *
 * @param config Pointer to the USB configuration structure.
 * @param data Pointer to the data to be written.
 * @param len Length of the data to be written.
 * 
 * @return ESP_OK if configuration is valid for the serial port.
 */
esp_err_t usc_driver_write( const usc_config_t *config, 
                            const char *data, 
                            const size_t len);

/**
 * @brief Request a password using the USB driver.
 * @param config Pointer to the USB configuration structure.
 * @return ESP_OK if sending serial data from the port works as intended
 */
esp_err_t usc_driver_request_password(usc_config_t *config);

/**
 * @brief Ping the USB driver.
 * @param config Pointer to the USB configuration structure.
 * @return
 *     - ESP_OK: Success
 * 
 *     - ESP_FAIL: Failed
 */
esp_err_t usc_driver_ping(usc_config_t *config);

void uart_port_config_deinit(uart_port_config_t *uart_config);

/**
 * @brief Deinitialize the USB driver.
 * @param[in] config Pointer to the USB configuration structure.
 * @return ESP_OK if driver is valid
 */
esp_err_t usc_driver_deinit(serial_input_driver_t *config);

void clear_serial_memory(Queue *serial_memory);

/**
 * @brief Deinitialize all USB drivers.
 * @param config Pointer to the USB configuration structure.
 * @param action Pointer to the event callback function.
 * @param i Index of the overdriver to be set.
 * @note The index should be between 0 and OVERDRIVER_MAX - 1.
 * @return ESP_OK if all arguments are valid
 */
esp_err_t usc_set_overdrive( const usc_config_t *config, 
                             const usc_event_cb_t action, 
                             const int i);

/**
 * @brief Set the overdrive driver.
 * @param driver Pointer to the serial input driver structure.
 * @param i Index of the overdriver to be set.
 * @note The index should be between 0 and OVERDRIVER_MAX - 1.
 * @return ESP_OK if the index is valid and the driver is set successfully.
 */
esp_err_t overdrive_usc_driver( serial_input_driver_t *driver, 
                                const int i);
/**
 * @brief Deinitialize all USB drivers.
 * @param drivers Pointer to the serial input drivers structure.
 * @return ESP_OK if there was no issue in deinitializing the overdrivers
 */


/**
 * @brief Prints the configurations of all initialized drivers.
 * Iterates through each driver and logs its configuration details if it is initialized.
 */
void usc_print_driver_configurations(void);

/**
 * @brief Prints the configurations of all overdrivers.
 * Iterates through each overdriver and logs its configuration details if it is initialized.
 */
void usc_print_overdriver_configurations(void);

#ifdef __cplusplus
}
#endif

#endif // __USB_DRIVER_H