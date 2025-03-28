#ifndef __USB_DRIVER_H
#define __USB_DRIVER_H

#include "memory_pool.h"
#include "esp_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_MAX              2

#define SERIAL_KEY      "123456789"
#define REQUEST_KEY     "GSKx" // used for requesting the passcode for the driver
#define PING            "ping"

#define SERIAL_DATA_SIZE      126

typedef struct Node {
    serial_data_t data; // change in the future
    struct Node *next;
} Node;

#define SIZE_OF_SERIAL_MEMORY_BLOCK    (sizeof(Node)) // must be defined

/**
 * @brief Type definition for USB event callback function.
 *
 * @param void *arg: Argument passed to the callback.
 */
typedef void (*usc_event_cb_t)(void *);

/**
 * @brief Type definition for USB action callback function.
 *
 * @param void *arg: Argument passed to the callback.
 */
typedef void (*usc_data_process_t)(void *);

/**
 * @brief Type definition for driver name.
 */
typedef char driver_name_t[20];

/**
 * @brief Type definition for serial key.
 */
typedef char serial_key_t[10];

/**
 * @brief 
 *   NOT_CONNECTED
 *   
 *   CONNECTED
 *   
 *   DISCONNECTED  
 *   
 *   ERROR      
 *   
 *   DATA_RECEIVED
 *     
 *   DATA_SENT
 *        
 *   DATA_SEND_ERROR 
 *    
 *   DATA_RECEIVE_ERROR
 * 
 *   DATA_SEND_TIMEOUT
 *   
 *   DATA_RECEIVE_TIMEOUT
 * 
 *   DATA_SEND_COMPLETE
 * 
 *   DATA_RECEIVE_COMPLETE
 * 
 *   TIME_OUT      
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
 * 
 * uart_port_config_t - uart_config
 * 
 * driver_name_t - driver_name
 */
typedef struct {
    uart_port_config_t uart_config; ///< UART configuration structure.
    driver_name_t driver_name; ///< Name of the driver.
    
    bool has_access; ///< Flag indicating if access is granted.

    memory_block_t *data; // stored data from port to driver

    usc_status_t status; ///< Current status of the USB connection.

    baud_rate_t baud_rate; ///< Baud rate of the UART (not implemented yet).
} usc_config_t;

/**
 * @brief Structure for serial input drivers.
 */
typedef struct {
    usc_config_t config; ///< Array of USB configurations
    usc_data_process_t driver_action; ///< Action callback for the overdrive driver.
} serial_input_driver_t;

/**
 * @brief Type definition for overdriver size.
 */
typedef unsigned int overdriver_size_t;

/**
 * @brief Initialize the USB driver.
 *        Index is from 0 to DRIVER_MAX - 1 (0 to 1)
 * 
 * @param[in] config Pointer to the USB configuration structure.
 * @param[in] event_cb Pointer to the event callback function.
 * @param[out] dfomdf dfomd
 * @return
 *     - ESP_OK: Success
 * 
 *     - ESP_FAIL: Failed
 */
esp_err_t usb_driver_init(usc_config_t *config, uart_config_t uart_config, usc_data_process_t driver_process, int i);

/**
 * @brief Prints the configurations of all initialized drivers.
 * 
 * Iterates through each driver and logs its configuration details if it is initialized.
 */
void usb_print_driver_configurations(void);

/**
 * @brief Prints the configurations of all overdrivers.
 * 
 * Iterates through each overdriver and logs its configuration details if it is initialized.
 */
void usb_print_overdriver_configuration(void);

/**
 * @brief Write data using the USB driver.
 *
 * @param[in] config Pointer to the USB configuration structure.
 * @param[in] data Pointer to the data to be written.
 * @param[in] len Length of the data to be written.
 *
 * @return
 *     - ESP_OK: Success
 * 
 *     - ESP_FAIL: Failed
 */
esp_err_t usb_driver_write(usc_config_t *config, serial_data_ptr_t data, size_t len);

/**
 * @brief Request a password using the USB driver.
 *
 * @param[in] config Pointer to the USB configuration structure.
 *
 * @return
 *     - ESP_OK: Success
 * 
 *     - ESP_FAIL: Failed
 */
esp_err_t usb_driver_request_password(usc_config_t *config);

/**
 * @brief Ping the USB driver.
 *
 * @param[in] config Pointer to the USB configuration structure.
 *
 * @return
 *     - ESP_OK: Success
 * 
 *     - ESP_FAIL: Failed
 */
esp_err_t usb_driver_ping(usc_config_t *config);

void uart_port_config_deinit(uart_port_config_t *uart_config);

/**
 * @brief Deinitialize the USB driver.
 *
 * @param[in] config Pointer to the USB configuration structure.
 *
 * @return
 *     - ESP_OK: Success
 * 
 *     - ESP_FAIL: Failed
 */
esp_err_t usb_driver_deinit(serial_input_driver_t *config);

esp_err_t usb_set_overdrive(usc_config_t *config, usc_event_cb_t action, int i);

esp_err_t overdrive_usb_driver(serial_input_driver_t *driver, int i);
/**
 * @brief Deinitialize all USB drivers.
 *
 * @param[in] drivers Pointer to the serial input drivers structure.
 *
 * @return
 *     - ESP_OK: Success

 *     - ESP_FAIL: Failed
 */
esp_err_t usb_overdriver_deinit_all(void);

#ifdef __cplusplus
}
#endif

#endif // __USB_DRIVER_H