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

// Validate UART configuration
static void check_valid_uart_config( const uart_config_t *uart_config,    
                                     const uart_port_config_t *port_config
) {
    xSemaphoreTake(driver_system.lock, portMAX_DELAY);
    bool v = ( ( driver_system.size + 1 ) >= DRIVER_MAX );
    xSemaphoreGive(driver_system.lock);

    if (v) {
        ESP_LOGE(TAG, "Invalid driver index");
        set_microusc_system_code(USC_SYSTEM_ERROR);
        return;
    }

    if (OUTSIDE_SCOPE(port_config->port, UART_NUM_MAX)) {
        ESP_LOGE(TAG, "Invalid UART port");
        set_microusc_system_code(USC_SYSTEM_ERROR);
        return;
    }

    UBaseType_t i = getCurrentEmptyDriverIndex();

    vTaskDelay(LOOP_DELAY_MS); // 10ms delay

    // Initialize UAR
    uart_init(*port_config, *uart_config);
}

struct usc_driver_t *getLastDriver(void) {
    struct usc_driverList *node = list_last_entry(&driver_system.driver_list.list, struct usc_driverList, list); // use the tail of the list (the one that has been recently added)
    return (node != NULL) ? &node->driver : NULL;
}

esp_err_t usc_driver_init( const char *const driver_name,
                           const uart_config_t uart_config,
                           const uart_port_config_t port_config,
                           const usc_process_t driver_process,
                           const stack_size_t stack_size
) {
    if (driver_process == NULL) {
        ESP_LOGE(TAG, "driver_process cannot be NULL");
        set_microusc_system_code(USC_SYSTEM_ERROR);
        return ESP_ERR_INVALID_ARG;
    }

    check_valid_uart_config(&uart_config, &port_config);
    addSingleDriver(driver_name, uart_config, port_config, driver_process, stack_size);

    SemaphoreHandle_t system_lock = driver_system.lock;
    xSemaphoreTake(system_lock, portMAX_DELAY); // WILL NOT CONTINUE IF IT DOES NOT RECIEVE AUTHERIZATION 1 IN
    struct usc_driver_t *current_driver = getLastDriver(); // check if it was succesfully linked to the sub system
    if (current_driver == NULL) {
        ESP_LOGE(TAG, "Failed to get the last driver in the system driver manager");
        xSemaphoreGive(system_lock); // 1 OUT
        set_microusc_system_code(USC_SYSTEM_ERROR);
        return ESP_ERR_NO_MEM;
    }

    if (!xSemaphoreTake(current_driver->sync_signal, SEMAPHORE_WAIT_TIME)) { // Wait for the mutex lock 2IN
        ESP_LOGE(TAG, "Failed to take semaphore");
        xSemaphoreGive(system_lock); // 1 OUT
        set_microusc_system_code(USC_SYSTEM_ERROR);
        return ESP_ERR_INVALID_STATE; // Failed to take semaphore
    }

    xSemaphoreGive(system_lock); // 1 OUT
    xSemaphoreGive(current_driver->sync_signal); // Release the mutex lock 2 OUT
    
    #ifdef MICROUSC_DEBUG_MEMORY_USAGE
    set_microusc_system_code(USC_SYSTEM_MEMORY_USAGE);
    #endif
    
    return ESP_OK;
}

/**
 * @brief Write data to a USC (MicroUSC) driver using specified configuration.
 *
 * This function transmits a data buffer to the hardware interface defined by the provided
 * usc_config_t driver settings. It handles the details of serial or peripheral communication,
 * ensuring the data is sent according to the driver’s configuration parameters.
 *
 * @param driver_setting Pointer to the USC driver configuration structure (usc_config_t).
 * @param data Pointer to the data buffer to be transmitted.
 * @param len Number of bytes to write from the data buffer.
 * @return
 *   - ESP_OK: Data was written successfully.
 *   - ESP_FAIL or error code: Transmission failed (see implementation for details).
 */
static esp_err_t usc_driver_write( const struct usc_driver_t *driver,
                                   const char *data,
                                   const size_t len
) {
    if (driver->uart_config.baud_rate < CONFIGURED_BAUDRATE) { 
        const TickType_t delay = ( driver->uart_config.baud_rate / CONFIGURED_BAUDRATE ) / portTICK_PERIOD_MS;
        literate_bytes(data, const char, len) {
            if (uart_write_bytes(driver->port_config.port, begin /* from literate_bytes */, 1) == -1) {
                return ESP_FAIL;
            }
            // delay in order to make sure the other device (reciever) that has a slower baud rate
            // is able to recieve every serial data given. Otherwise data may be lost
            vTaskDelay(delay);
        }
        return ESP_OK;
    }
    return (uart_write_bytes(driver->port_config.port, data, len) == -1) ? ESP_FAIL : ESP_OK;
}

static __always_inline esp_err_t usc_driver_send_helper( const struct usc_driver_t *driver,
                                                         const char *data,
                                                         const size_t len
) {
    memcpy((driver->buffer.memory + 1), data, len);
    return usc_driver_write(driver, (const char *)driver->buffer.memory, driver->buffer.size);
}

static __always_inline esp_err_t usc_driver_request_password(struct usc_driver_t *driver) 
{
    return usc_driver_send_helper(driver, (const char *)REQUEST_KEY.bytes, sizeof(REQUEST_KEY));
}

static __always_inline esp_err_t usc_driver_ping(struct usc_driver_t *driver)
{
    return usc_driver_send_helper(driver, (const char *)PING.bytes, sizeof(PING));
}

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

// Combine 4 bytes in order into 32-bit value, reverses the value that it has recieved
static __always_inline uint32_t parse_data(const uint8_t *const data) 
{
    union uint32_4_uint8_t rx;
    memcpy(rx.bytes, (data + 1), sizeof(rx));
    return rx.value;
}

// needs to be changed to use the queue
usc_status_t handle_serial_key(struct usc_driver_t *driver, const UBaseType_t i)
{
    if (usc_driver_request_password(driver) != ESP_OK) { // Request serial key
        ESP_LOGE(TAG, "Failed to send serial");
        return DATA_SEND_ERROR; // doesn't need system interface
    }

    SERIAL_RECIEVE_DELAY();

    // 1 is added for the NULL terminator which is junk
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
                // do nothing
                break;
        }
    }
    return TIME_OUT; // doesn't need system interface
}

static __always_inline void maintain_connection(struct usc_driver_t *driver) 
{
    usc_driver_ping(driver);
    SERIAL_RECIEVE_DELAY();
}

// needs to be finished
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

// do not rename
void usc_driver_read_task(void *pvParameters)
{
    const UBaseType_t index = uxTaskPriorityGet(NULL) - TASK_PRIORITY_START; // gets priority ID of the task, more temportary
    struct usc_driver_t *driver = (struct usc_driver_t *)pvParameters; // make a copy inside the function
    SemaphoreHandle_t sync_signal = driver->sync_signal; // Get the mutex lock for the driver
    bool *active = &driver->uart_reader.active; // Check if the driver has access
    bool *hasAccess = &driver->has_access;
    // prioirty should give the id number minus the offset of the task
    ESP_LOGI(TASK_TAG, "Priority %u\n", (index + TASK_PRIORITY_START)); // Debugging line to check task name and priority`
    ESP_LOGI(TASK_TAG, "Task status: %d\n", *active);
    vTaskDelay(LOOP_DELAY_MS); // 10ms delay
    
    //#if (NEEDS_SERIAL_KEY == 1)
    while (1) {
        if (xSemaphoreTake(sync_signal, portMAX_DELAY) == pdTRUE) {
            bool needs_check = *active && !(*hasAccess);
            if (needs_check) {
                driver->status = handle_serial_key(driver, index); // Check if the serial key is valid
                if (driver->status != CONNECTED) {
                    ESP_LOGW(TASK_TAG, "Serial key check failed, retrying...");  // Debugging line to check if the serial key check failed
                }
                else {
                    driver->has_access = true;
                }
            }

            // driver_isr_trigger(driver);
            xSemaphoreGive(sync_signal); // finish this part

            if (!needs_check) { // break if a condition is different
                break;
            }

            vTaskDelay(LOOP_DELAY_MS); // 10ms delay
        }
    }
    //#endif

   while (1) {
        if (xSemaphoreTake(sync_signal, portMAX_DELAY) == pdTRUE) {
            driver->status = process_data(driver, index);
            ESP_LOGI(TASK_TAG, "Task %d is running", index);
            bool should_exit = (*active == false);
            driver_isr_trigger(driver);
            taskYIELD(); // Yield to allow other tasks to run
            // xSemaphoreGive(sync_signal);
            if (should_exit) break;
        }
        vTaskDelay(LOOP_DELAY_MS); // Delay outside the critical section
    }

    ESP_LOGI(TASK_TAG, "Task %s is terminating...\n", driver->driver_name); // Debugging line to check task name and priority
    vTaskDelay(LOOP_DELAY_MS); // 10ms delay
    vTaskDelete(NULL); // Delete the task
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