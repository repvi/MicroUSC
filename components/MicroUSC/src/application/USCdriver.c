#include "MicroUSC/system/manager.h"
#include "MicroUSC/system/memory_pool.h"
#include "MicroUSC/USCdriver.h"
#include "MicroUSC/synced_driver/esp_uart.h"
#include "MicroUSC/internal/system/bit_manip.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/internal/driverList.h"
#include "MicroUSC/internal/uscdef.h"
#include "debugging/speed_test.h"
#include "string.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_check.h"

#define TAG                "[USC DRIVER]"
#define TASK_TAG           "[DRIVER READER]"

#define REQUEST_KEY_VAL ( uint32_t ) ( 0x64 ) // send data to request for password (idle)
#define PING_VAL        ( uint32_t ) ( 0x63 ) // ping to the other device, (NOT USED)
#define SEND_KEY_VAL    ( uint32_t ) ( 1234 ) // send the other device's password
#define SERIAL_KEY_VAL  ( uint32_t ) ( 1234 ) // the internal password for this device

#define BYTE_TYPE  uint32_t
union uint32_4_uint8_t {
    BYTE_TYPE value;
    uint8_t bytes[sizeof(BYTE_TYPE)];
};

const union uint32_4_uint8_t
REQUEST_KEY = {.value = REQUEST_KEY_VAL}, 
PING = {.value = PING_VAL}, 
SEND_KEY = {.value = SEND_KEY_VAL}, 
SERIAL_KEY = {.value = SERIAL_KEY_VAL};

#define PASSWORD_PING_DELAY pdMS_TO_TICKS(50) // 50 miliseconds delay
#define SERIAL_INPUT_DELAY pdMS_TO_TICKS(10) // 10 milisecond delay

#define SERIAL_RECIEVE_DELAY() vTaskDelay(pdMS_TO_TICKS(SERIAL_REQUEST_DELAY_MS)) // Wait for response

#define SERIAL_DATA_STORAGE_CAPACITY  256

#define buf_SIZE ( sizeof( uint32_t ) + 1 )

static esp_err_t check_valid_uart_config( const uart_config_t *uart_config,    
                                          const uart_port_config_t *port_config
) {
    xSemaphoreTake(driver_system.lock, portMAX_DELAY);
    bool v = ( ( driver_system.size + 1 ) >= DRIVER_MAX );
    xSemaphoreGive(driver_system.lock);

    if (v) {
        send_microusc_system_status(USC_SYSTEM_ERROR);
        return ESP_ERR_INVALID_ARG;
    }

    if (OUTSIDE_SCOPE(port_config->port, UART_NUM_MAX)) {
        send_microusc_system_status(USC_SYSTEM_ERROR);
        return ESP_ERR_INVALID_ARG;
    }

    vTaskDelay(LOOP_DELAY_MS); /* 10ms delay */

    /* Initialize UART hardware */
    return uart_init(*port_config, *uart_config);
}

struct usc_driver_t *getLastDriver(void) {
    struct usc_driverList *node = list_last_entry(&driver_system.driver_list.list, struct usc_driverList, list); // use the tail of the list (the one that has been recently added)
    return (node != NULL) ? &node->driver : NULL;
}

uscDriverHandler usc_driver_create( 
                              const char *const driver_name,
                              const uart_config_t uart_config,
                              const uart_port_config_t port_config
) {
    /* Validate UART configuration and add driver */
    ESP_RETURN_ON_FALSE((check_valid_uart_config(&uart_config, &port_config) == ESP_OK), NULL, TAG, "Invalid UART configuration");
    addSingleDriver(driver_name, uart_config, port_config);
    SemaphoreHandle_t system_lock = driver_system.lock;
    ESP_RETURN_ON_FALSE(xSemaphoreTake(system_lock, portMAX_DELAY) == pdTRUE, NULL, TAG, "Failed to take system lock");
    uscDriverHandler current_driver = getLastDriver(); /* check if it was succesfully linked to the sub system */
    if (current_driver == NULL) {
        ESP_LOGE(TAG, "Failed to get the last driver in the system driver manager");
        goto cleanup_fail;
    }

    xSemaphoreGive(system_lock); /* Release system lock */
    
    #ifdef MICROUSC_DEBUG_MEMORY_USAGE
    send_microusc_system_status(USC_SYSTEM_MEMORY_USAGE);
    #endif
    
    return current_driver;

cleanup_fail:
    xSemaphoreGive(system_lock);
    // send_microusc_system_status(USC_SYSTEM_ERROR);
    return NULL;
}

static __always_inline esp_err_t usc_driver_send_helper( const struct usc_driver_t *driver,
                                                         const char *data,
                                                         const size_t len
) {
    /* Copy data into driver's buffer at offset 1 */
    memcpy((driver->buffer.memory + 1), data, len);
    /* Write the entire buffer to UART */
    return (uart_write_bytes(driver->port_config.port, (const char *)driver->buffer.memory, driver->buffer.size) == -1) ? ESP_FAIL : ESP_OK;
}

static __always_inline esp_err_t usc_driver_request_password(struct usc_driver_t *driver) 
{
    return usc_driver_send_helper(driver, (const char *)REQUEST_KEY.bytes, sizeof(REQUEST_KEY));
}

static __always_inline esp_err_t usc_driver_send_password(struct usc_driver_t *driver) 
{
    return usc_driver_send_helper(driver, (const char *)SEND_KEY.bytes, sizeof(SEND_KEY));
}

__always_inline esp_err_t usc_driver_send_data(uscDriverHandler driver, uint32_t data)
{
    union uint32_4_uint8_t bytes_4; // for safety
    bytes_4.value = data;

    xSemaphoreTake(driver->sync_signal, portMAX_DELAY);
    esp_err_t c = usc_driver_send_helper(driver, (const char *)bytes_4.bytes, sizeof(bytes_4));
    xSemaphoreGive(driver->sync_signal);
    return c;
}

static __always_inline uint32_t parse_data(const uint8_t *const data) 
{
    union uint32_4_uint8_t rx;
    memcpy(rx.bytes, (data + 1), sizeof(rx));
    return rx.value;
}

// needs to be changed to use the queue
usc_status_t handle_serial_key(struct usc_driver_t *driver)
{
    if (usc_driver_request_password(driver) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send serial");
        return DATA_SEND_ERROR;
    }

    SERIAL_RECIEVE_DELAY();

    /* Read the serial key from UART */
    uint8_t *key = uart_read(driver->port_config.port, driver->buffer.memory, driver->buffer.size, PASSWORD_PING_DELAY);
    if (key != NULL) {
        #ifdef MICROUSC_UART_DEBUG
        ESP_LOGI(TAG, "Serial key: %u %u %u %u", key[1], key[2], key[3], key[4]);
        #endif
        uint32_t parsed_data = parse_data(key);
        //ESP_LOGI(TAG, "Parsed value: %lu", parsed_data);

        switch (parsed_data) {
            case SERIAL_KEY_VAL:
                return CONNECTED;
            case REQUEST_KEY_VAL: {
                esp_err_t err = usc_driver_send_password(driver);
                if (err != ESP_OK) {
                    return ERROR;
                }
            }   
                break;
            default:
                /* Do nothing for unknown key */
                break;
        }
    }
    return TIME_OUT;
}

static usc_status_t process_data(struct usc_driver_t *driver, usc_command_t *command)
{
    uint8_t *temp_data = uart_read( driver->port_config.port, 
                                    driver->buffer.memory, 
                                    driver->buffer.size, 
                                    SERIAL_INPUT_DELAY
                                  );
    if (temp_data != NULL) {
        uint32_t data = parse_data(temp_data);
        *command = data;
        // dataStorageQueue_add(driver->data, data); // Add the data to the queue
        #ifdef MICROUSC_UART_DEBUG
        ESP_LOGI(TAG, "Got: %lu", data);
        #endif
        return DATA_RECEIVED;
    }
    return DATA_RECEIVE_ERROR; // doesn't need system interface
}

bool usc_driver_get_data_needs_access(uscDriverHandler driver, usc_command_t *command)
{
    SemaphoreHandle_t sync_signal = driver->sync_signal;
    ESP_RETURN_ON_FALSE(xSemaphoreTake(sync_signal, portMAX_DELAY) == pdTRUE, false, TAG, "Failed to take semaphore");

    if (!driver->has_access) {
        driver->status = handle_serial_key(driver); /* Check if the serial key is valid */
        if (driver->status == CONNECTED) {
            driver->has_access = true;
        }
        else {
            // ESP_LOGW(TASK_TAG, "Serial key check failed");
            goto failed;
        }
    }

    driver->status = process_data(driver, command);
    if (driver->status != DATA_RECEIVED) {
        goto failed;
    }

    // ESP_LOGI(TASK_TAG, "Task %d is running", index);
    return true;

    // Failed to get data, cleanup and return false
failed:
    xSemaphoreGive(sync_signal);
    return false;
}

bool usc_driver_get_data(uscDriverHandler driver, usc_command_t *command)
{
    SemaphoreHandle_t sync_signal = driver->sync_signal;
    ESP_RETURN_ON_FALSE(xSemaphoreTake(sync_signal, portMAX_DELAY) == pdTRUE, false, TAG, "Failed to take semaphore");

    driver->status = process_data(driver, command);
    if (driver->status != DATA_RECEIVED) {
        goto failed;
    }

    xSemaphoreGive(sync_signal);
    return true;

failed:
    xSemaphoreGive(sync_signal);
    return false;
}

bool usc_driver_has_access(uscDriverHandler driver)
{
    bool access = false;
    if (xSemaphoreTake(driver->sync_signal, portMAX_DELAY) == pdTRUE) {
        access = driver->has_access;
        xSemaphoreGive(driver->sync_signal);
    }
    return access;
}