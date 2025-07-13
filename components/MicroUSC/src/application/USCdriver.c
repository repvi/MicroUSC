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

/**
 * @brief Validates UART configuration and initializes UART hardware.
 *
 * Checks if the driver index is valid and if the UART port is within range.
 * If valid, initializes the UART hardware with the provided configuration.
 *
 * @param uart_config Pointer to UART configuration structure.
 * @param port_config Pointer to UART port configuration structure.
 */
static void check_valid_uart_config( const uart_config_t *uart_config,    
                                     const uart_port_config_t *port_config
) {
    xSemaphoreTake(driver_system.lock, portMAX_DELAY);
    bool v = ( ( driver_system.size + 1 ) >= DRIVER_MAX );
    xSemaphoreGive(driver_system.lock);

    if (v) {
        ESP_LOGE(TAG, "Invalid driver index");
        send_microusc_system_status(USC_SYSTEM_ERROR);
        return;
    }

    if (OUTSIDE_SCOPE(port_config->port, UART_NUM_MAX)) {
        ESP_LOGE(TAG, "Invalid UART port");
        send_microusc_system_status(USC_SYSTEM_ERROR);
        return;
    }

    vTaskDelay(LOOP_DELAY_MS); /* 10ms delay */

    /* Initialize UART hardware */
    uart_init(*port_config, *uart_config);
}

/**
 * @brief Retrieves the last driver added to the driver system list.
 *
 * @return Pointer to the last usc_driver_t, or NULL if the list is empty.
 */
struct usc_driver_t *getLastDriver(void) {
    struct usc_driverList *node = list_last_entry(&driver_system.driver_list.list, struct usc_driverList, list); // use the tail of the list (the one that has been recently added)
    return (node != NULL) ? &node->driver : NULL;
}

esp_err_t usc_driver_install( const char *const driver_name,
                              const uart_config_t uart_config,
                              const uart_port_config_t port_config,
                              const usc_process_t driver_process,
                              const stack_size_t stack_size
) {
    if (driver_process == NULL) {
        ESP_LOGE(TAG, "driver_process cannot be NULL");
        send_microusc_system_status(USC_SYSTEM_ERROR);
        return ESP_ERR_INVALID_ARG;
    }

    /* Validate UART configuration and add driver */
    check_valid_uart_config(&uart_config, &port_config);
    addSingleDriver(driver_name, uart_config, port_config, driver_process, stack_size);

    SemaphoreHandle_t system_lock = driver_system.lock;
    xSemaphoreTake(system_lock, portMAX_DELAY); /* Acquire system lock */
    struct usc_driver_t *current_driver = getLastDriver(); /* check if it was succesfully linked to the sub system */
    if (current_driver == NULL) {
        ESP_LOGE(TAG, "Failed to get the last driver in the system driver manager");
        xSemaphoreGive(system_lock);
        send_microusc_system_status(USC_SYSTEM_ERROR);
        return ESP_ERR_NO_MEM;
    }

    /* Acquire driver's sync semaphore */
    if (!xSemaphoreTake(current_driver->sync_signal, SEMAPHORE_WAIT_TIME)) {
        ESP_LOGE(TAG, "Failed to take semaphore");
        xSemaphoreGive(system_lock);
        send_microusc_system_status(USC_SYSTEM_ERROR);
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreGive(system_lock); /* Release system lock */
    xSemaphoreGive(current_driver->sync_signal); /* Release driver's sync semaphore */
    
    #ifdef MICROUSC_DEBUG_MEMORY_USAGE
    send_microusc_system_status(USC_SYSTEM_MEMORY_USAGE);
    #endif
    
    return ESP_OK;
}

/**
 * @brief Writes data to the UART port associated with the driver.
 *
 * @param driver Pointer to the driver structure.
 * @param data Pointer to the data buffer to send.
 * @param len Number of bytes to send.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static esp_err_t usc_driver_write( const struct usc_driver_t *driver,
                                   const char *data,
                                   const size_t len
) {
    /* Write data to UART; return ESP_OK if successful, ESP_FAIL otherwise */
    return (uart_write_bytes(driver->port_config.port, data, len) == -1) ? ESP_FAIL : ESP_OK;
}

/**
 * @brief Helper function to send a data buffer using the driver's buffer.
 *
 * Copies the data into the driver's buffer (offset by 1), then writes it to UART.
 *
 * @param driver Pointer to the driver structure.
 * @param data Pointer to the data buffer to send.
 * @param len Number of bytes to send.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static __always_inline esp_err_t usc_driver_send_helper( const struct usc_driver_t *driver,
                                                         const char *data,
                                                         const size_t len
) {
    /* Copy data into driver's buffer at offset 1 */
    memcpy((driver->buffer.memory + 1), data, len);
    /* Write the entire buffer to UART */
    return usc_driver_write(driver, (const char *)driver->buffer.memory, driver->buffer.size);
}

/**
 * @brief Sends a password request command to the driver.
 *
 * @param driver Pointer to the driver structure.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static __always_inline esp_err_t usc_driver_request_password(struct usc_driver_t *driver) 
{
    return usc_driver_send_helper(driver, (const char *)REQUEST_KEY.bytes, sizeof(REQUEST_KEY));
}

/**
 * @brief Sends a ping command to the driver.
 *
 * @param driver Pointer to the driver structure.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static __always_inline esp_err_t usc_driver_ping(struct usc_driver_t *driver)
{
    return usc_driver_send_helper(driver, (const char *)PING.bytes, sizeof(PING));
}

/**
 * @brief Sends the password to the driver.
 *
 * @param driver Pointer to the driver structure.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static __always_inline esp_err_t usc_driver_send_password(struct usc_driver_t *driver) 
{
    return usc_driver_send_helper(driver, (const char *)SEND_KEY.bytes, sizeof(SEND_KEY));
}

__always_inline esp_err_t usc_send_data(uscDriverHandler driver, uint32_t data)
{
    union uint32_4_uint8_t bytes_4; // for safety
    bytes_4.value = data;

    xSemaphoreTake(driver->sync_signal, portMAX_DELAY);
    esp_err_t c = usc_driver_send_helper(driver, (const char *)bytes_4.bytes, sizeof(bytes_4));
    xSemaphoreGive(driver->sync_signal);
    return c;
}

/**
 * @brief Parses a 32-bit value from a received data buffer.
 *
 * Copies 4 bytes from the buffer (offset by 1) and returns the combined value.
 *
 * @param data Pointer to the received data buffer.
 * @return Parsed 32-bit value.
 */
static __always_inline uint32_t parse_data(const uint8_t *const data) 
{
    union uint32_4_uint8_t rx;
    memcpy(rx.bytes, (data + 1), sizeof(rx));
    return rx.value;
}

// needs to be changed to use the queue
usc_status_t handle_serial_key(struct usc_driver_t *driver, const UBaseType_t i)
{
    if (usc_driver_request_password(driver) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send serial");
        return DATA_SEND_ERROR;
    }

    SERIAL_RECIEVE_DELAY();

    /* Read the serial key from UART */
    uint8_t *key = uart_read( driver->port_config.port, 
                              driver->buffer.memory, 
                              driver->buffer.size, 
                              PASSWORD_PING_DELAY
                            );
    if (key != NULL) {
        ESP_LOGI(TAG, "Serial key: %u %u %u %u", key[1], key[2], key[3], key[4]);
        uint32_t parsed_data = parse_data(key);
        ESP_LOGI(TAG, "Parsed value: %lu", parsed_data);

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

/**
 * @brief Sends a ping to the driver to maintain the connection.
 *
 * @param driver Pointer to the driver structure.
 */
static __always_inline void maintain_connection(struct usc_driver_t *driver) 
{
    usc_driver_ping(driver);
    SERIAL_RECIEVE_DELAY();
}

/**
 * @brief Reads and processes incoming data from the driver.
 *
 * Reads data from UART, parses it, and stores it in the driver's data queue if valid.
 *
 * @param driver Pointer to the driver structure.
 * @param i Index of the driver (unused).
 * @return Status code indicating if data was received.
 */
static usc_status_t process_data(struct usc_driver_t *driver, const UBaseType_t i)
{
    uint8_t *temp_data = uart_read( driver->port_config.port, 
                                    driver->buffer.memory, 
                                    driver->buffer.size, 
                                    SERIAL_INPUT_DELAY
                                  );
    if (temp_data != NULL) {
        uint32_t data = parse_data(temp_data);
        if (data != 0) {
            dataStorageQueue_add(driver->data, data); // Add the data to the queue
            ESP_LOGI(TAG, "Stored: %lu", data);
            return DATA_RECEIVED;
        }
    }
    return DATA_RECEIVE_ERROR; // doesn't need system interface
}

void usc_driver_read_task(void *pvParameters)
{
    const UBaseType_t index = uxTaskPriorityGet(NULL) - TASK_PRIORITY_START; /* gets priority ID of the task, more temportary */
    struct usc_driver_t *driver = (struct usc_driver_t *)pvParameters; 
    SemaphoreHandle_t sync_signal = driver->sync_signal;
    bool *active = &driver->uart_reader.active;
    bool *hasAccess = &driver->has_access;

    ESP_LOGI(TASK_TAG, "Priority %u\n", (index + TASK_PRIORITY_START));
    ESP_LOGI(TASK_TAG, "Task status: %d\n", *active);
    vTaskDelay(LOOP_DELAY_MS);
    
    while (1) {
        if (xSemaphoreTake(sync_signal, portMAX_DELAY) == pdTRUE) {
            bool needs_check = *active && !(*hasAccess);
            if (needs_check) {
                driver->status = handle_serial_key(driver, index); /* Check if the serial key is valid */
                if (driver->status != CONNECTED) {
                    ESP_LOGW(TASK_TAG, "Serial key check failed, retrying...");
                }
                else {
                    driver->has_access = true;
                }
            }

            /* Release the semaphore after handshake */
            xSemaphoreGive(sync_signal);

            if (!needs_check) {
                break;
            }

            vTaskDelay(LOOP_DELAY_MS);
        }
    }


    /* Main data processing loop */
   while (1) {
        if (xSemaphoreTake(sync_signal, portMAX_DELAY) == pdTRUE) {
            driver->status = process_data(driver, index);
            ESP_LOGI(TASK_TAG, "Task %d is running", index);
            bool should_exit = (*active == false);
            driver_isr_trigger(driver);
            taskYIELD(); /* Yield to allow other tasks to run */
            // xSemaphoreGive(sync_signal);
            if (should_exit) break;
        }
        vTaskDelay(LOOP_DELAY_MS);
    }

    ESP_LOGI(TASK_TAG, "Task %s is terminating...\n", driver->driver_name);
    vTaskDelay(LOOP_DELAY_MS);
    vTaskDelete(NULL); /* Delete the task */
}

uint32_t usc_driver_get_data(uscDriverHandler driver)
{
    uint32_t data = 0;
    if (xSemaphoreTake(driver->sync_signal, portMAX_DELAY) == pdTRUE) {
        if (driver->has_access) {
            data = dataStorageQueue_top(driver->data);
        }
        xSemaphoreGive(driver->sync_signal);
    }
    return data;
}