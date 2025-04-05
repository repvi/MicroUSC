#ifndef __USB_DRIVER_H
#define __USB_DRIVER_H

#include "memory_pool.h"
#include "esp_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CURRENT_VERSION_MAJOR             (0)
#define CURRENT_VERSION_MINOR             (5)
#define CURRENT_VERSION_PATCH             (25)

#define to_string(x)        #x
#define USC_Version()       to_string(CURRENT_VERSION_MAJOR) "." \
                            to_string(CURRENT_VERSION_MINOR) "." \
                            to_string(CURRENT_VERSION_PATCH)

#undef TO_STRING

#define DRIVER_MAX              2

#define SERIAL_KEY      "123456789" // Password for the program
#define REQUEST_KEY     "GSKx" // Used for requesting the passcode for the driver
#define PING            "ping" // Used for making sure there is a connection

#define SERIAL_DATA_SIZE      126


#if defined(__GNUC__) || defined(__clang__)
    #define FUNCTION_ATTRIBUTES       (1)
#else
    #define FUNCTION_ATTRIBUTES       (0)
#endif

#define INSIDE_SCOPE(x, max) (0 <= (x) && (x) < (max))
#define OUTSIDE_SCOPE(x, max) ((x) < 0 || (max) <= (x))
#define developer_input(x) (x)

#if (FUNCTION_ATTRIBUTES == 1) 
    #define RUN_FIRST      __attribute__((constructor)) // probably might not use
    #define MALLOC         __attribute__((malloc)) // used for dynamic memory functions
    #define HOT            __attribute__((hot)) // for critical operations (Need most optimization)
    #define COLD           __attribute__((cold)) // not much much. Use less memory but slower execution of function. Used like in initializing
    
    #define OPT0         __attribute__((optimize("O0")))
    #define OPT1         __attribute__((optimize("O1")))
    #define OPT2         __attribute__((optimize("O2")))
    #define OPT3         __attribute__((optimize("O3")))
#else
    #define RUN_FIRST // Nothing is disabled
    #define MALLOC 
    #define HOT   
    #define COLD

    #define OPT0 
    #define OPT1 
    #define OPT2 
    #define OPT3 
#endif

#define OPTIMIZE_CONSTANT(x) \
    (__builtin_constant_p(x) ? optimize_for_constant(x) : general_case(x))


// DRAM_ATTR // put in IRAM, not in flash, not in PSRAM

// needs baud rate implementation
#ifdef CONFIG_ESP_CONSOLE_UART_BAUDRATE
    #define CONFIGURED_BAUDRATE       CONFIG_ESP_CONSOLE_UART_BAUDRATE
#endif
#ifndef CONFIG_ESP_CONSOLE_UART_BAUDRATE
    #define CONFIGURED_BAUDRATE       (-1)
#endif

ESP_STATIC_ASSERT(CONFIGURED_BAUDRATE != -1, "CONFIG_ESP_CONSOLE_UART_BAUDRATE is not defined!");

typedef struct stored_uart_data_t {
    serial_data_t data; // change in the future
    struct stored_uart_data_t *next;
} stored_uart_data_t;

#define SIZE_OF_SERIAL_MEMORY_BLOCK    (sizeof(stored_uart_data_t)) // must be defined

/**
 * @brief Type definition for USB event callback function.
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
    
    bool has_access; ///< Flag indicating if access is granted.

    Queue data; // UART_NUM_MAX is used as the size of the stored data in the port
    
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
 * @param config Pointer to the USB configuration structure.
 * @param event_cb Pointer to the event callback function.
 * @param dfomdf dfomd
 * @return
 *     - ESP_OK: Success
 * 
 *     - ESP_FAIL: Failed
 */
esp_err_t usc_driver_init(usc_config_t *config, uart_config_t uart_config, uart_port_config_t port_config, usc_data_process_t driver_process, int i);

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

/**
 * @brief Write data using the USB driver.
 *
 * @param config Pointer to the USB configuration structure.
 * @param data Pointer to the data to be written.
 * @param len Length of the data to be written.
 * @return ESP_OK if configuration is valid for the serial port.
 */
esp_err_t usc_driver_write(const usc_config_t *config, const char *data, const size_t len);

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

esp_err_t usc_set_overdrive(usc_config_t *config, usc_event_cb_t action, int i);

esp_err_t overdrive_usc_driver(serial_input_driver_t *driver, int i);
/**
 * @brief Deinitialize all USB drivers.
 * @param drivers Pointer to the serial input drivers structure.
 * @return ESP_OK if there was no issue in deinitializing the overdrivers
 */
void usc_overdriver_deinit_all(void);

#ifdef __cplusplus
}
#endif

#endif // __USB_DRIVER_H