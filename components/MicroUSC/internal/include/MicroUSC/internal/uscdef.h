#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/synced_driver/esp_uart.h"
#include "MicroUSC/synced_driver/atomic_sys_op.h"

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
        *(end) = (var) + (len); \
        (name) < (end); (name)++ \
    ) // used for the byte loop

#define define_iteration_with_semaphore(var, type, name, len) \
    for ( \
        type* (name) = (var), \
        *(end) = (var) + (len); \
        (name) < (end); \
        xSemaphoreGive((name)->sync_signal), (name)++ \
    ) /* used for the byte loop */ \
    if (xSemaphoreTake((name)->sync_signal, portMAX_DELAY) == pdTRUE) \
        for (bool hasSemaphore = true; hasSemaphore; hasSemaphore = false)
    
typedef char driver_name_t[20];
typedef char serial_key_t[10];
typedef bool init_safety;

#define STATIC_INIT_SAFETY  static init_safety
#define initZERO            false
#define initONE             true

#define DRIVER_NAME_SIZE              ( sizeof( driver_name_t ) )

/**
 * @brief Status of driver connection
 */
typedef enum { // used in the api
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
struct usc_config_t {
    uart_port_config_t uart_config; ///< UART configuration structure.
    Queue data; // UART_NUM_MAX is used as the size of the stored data in the port
    driver_name_t driver_name; ///< Name of the driver.
    usc_status_t status; ///< Current status of the USB connection.
    uint32_t baud_rate; ///< Baud rate of the UART (not implemented yet).
    bool has_access; ///< Flag indicating if access is granted.
}; // might not be needed outside of the file

struct usc_task_manager_t { // make the struct defined inside the c file instead in the future
    TaskHandle_t task_handle; // Task handle for the driver task (uart controller)
    TaskHandle_t action_handle; // Task handle for the action task (reader and writer)
    bool active;
};

struct usc_driver_base_t {
    struct usc_config_t driver_setting; ///< Array of USB configurations
    struct usc_task_manager_t driver_tasks; ///< Task manager for the drivers
    UBaseType_t priority;
};

// used in the kernel
struct usc_driver_t {
    struct usc_driver_base_t driver_storage;
    UBaseType_t priority; // the priority of the task of the driver
    SemaphoreHandle_t sync_signal;
};

#define INIT_USC_CONFIG_DEFAULT { \
        .baud_rate = 0, \
        .data = {0}, \
        .driver_name = "", \
        .has_access = false, \
        .status = NOT_CONNECTED, \
        .uart_config = {0} \
    }

#ifdef __cplusplus
}
#endif