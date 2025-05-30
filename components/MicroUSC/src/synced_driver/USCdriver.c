#include "MicroUSC/system/MicroUSC-internal.h"
#include "MicroUSC/system/memory_pool.h"
#include "MicroUSC/synced_driver/USCdriver.h"
#include "MicroUSC/internal/system/bit_manip.h"
#include "MicroUSC/synced_driver/esp_uart.h"
#include "MicroUSC/internal/driverList.h"
#include "MicroUSC/internal/uscdef.h"
#include "debugging/speed_test.h"
#include "string.h"
#include "esp_system.h"
#include "esp_log.h"

#define TAG                "[USC DRIVER]"
#define TASK_TAG           "[DRIVER READER]"

#define REQUEST_KEY                 0064
#define PING                        0063
#define SERIAL_KEY                  ( ( uint32_t ) (1234) )
#define SERIAL_REQUEST_SIZE         ( sizeof( uint32_t ) )

#define PASSWORD_PING_DELAY ( ( TickType_t ) ( 2000 / portTICK_PERIOD_MS ) ) // 2 seconds delay
#define SERIAL_INPUT_DELAY ( ( TickType_t ) ( 3000 / portTICK_PERIOD_MS ) ) // 3 second delay
#define SERIAL_RECIEVE_DELAY() vTaskDelay(SERIAL_REQUEST_DELAY_MS / portTICK_PERIOD_MS) // Wait for response

#define NOT_FOUND (( uint32_t ) ( -1 ) )

#define SERIAL_DATA_STORAGE_CAPACITY  256

QueueHandle_t uart_queue[DRIVER_MAX] = {NULL}; // Queue for UART data

uint8_t *buf = NULL;

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

    vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay

    // Initialize UAR
    uart_init(*port_config, *uart_config, &uart_queue[i], UART_QUEUE_SIZE);
}

struct usc_driver_t *getLastDriver(void) {
    struct usc_driverList *node = list_last_entry(&driver_system.driver_list.list, struct usc_driverList, list); // use the tail of the list (the one that has been recently added)
    return (node != NULL) ? &node->driver : NULL;
}

esp_err_t usc_driver_init( const char *const driver_name,
                           const uart_config_t uart_config,
                           const uart_port_config_t port_config,
                           const usc_data_process_t driver_process
) {
    if (driver_process == NULL) {
        ESP_LOGE(TAG, "driver_process cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    check_valid_uart_config(&uart_config, &port_config);

    UBaseType_t open_spot = getCurrentEmptyDriverIndexAndOccupy(); // retrieve the first empty bit

    addSingleDriver(driver_name, uart_config, port_config, open_spot);

    SemaphoreHandle_t system_lock = driver_system.lock;
    xSemaphoreTake(system_lock, portMAX_DELAY); // WILL NOT CONTINUE IF IT DOES NOT RECIEVE AUTHERIZATION 1 IN
    struct usc_driver_t *current_driver = getLastDriver(); // check if it was succesfully linked to the sub system
    if (current_driver == NULL) {
        ESP_LOGE(TAG, "Failed to get the last driver in the system driver manager");
        xSemaphoreGive(system_lock); // 1 OUT
        return ESP_ERR_NO_MEM;
    }

    if (!xSemaphoreTake(current_driver->sync_signal, SEMAPHORE_WAIT_TIME)) { // Wait for the mutex lock 2IN
        ESP_LOGE(TAG, "Failed to take semaphore");
        xSemaphoreGive(system_lock); // 1 OUT
        return ESP_ERR_INVALID_STATE; // Failed to take semaphore
    }

    create_usc_driver_task(current_driver, open_spot); // create the task for the serial driver implemented
    create_usc_driver_action_task(current_driver, driver_process, open_spot); // create task for the custom function (driver)

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
 *
 * @note This function is typically used in ESP32/ESP8266 embedded systems to abstract serial or
 *       peripheral communication, supporting robust driver and system configuration.
 *       Proper driver initialization must be performed before calling this function.
 */
static esp_err_t usc_driver_write( const usc_config_t *config,
                            const char *data,
                            const size_t len
) {
    if (config->baud_rate < CONFIGURED_BAUDRATE) { 
        const TickType_t delay = ( config->baud_rate / CONFIGURED_BAUDRATE ) / portTICK_PERIOD_MS;
        literate_bytes(data, const char, len) {
            if (uart_write_bytes(config->uart_config.port, begin /* from literate_bytes */, 1) == -1) {
                return ESP_FAIL;
            }
            vTaskDelay(delay); // Wait for response
        }
        return ESP_OK;
    }
    return (uart_write_bytes(config->uart_config.port, data, len) == -1) ? ESP_FAIL : ESP_OK;
}

static __always_inline esp_err_t usc_driver_request_password(usc_config_t *config) 
{
    return usc_driver_write(config, STRINGIFY(REQUEST_KEY), sizeof(REQUEST_KEY));
}

static __always_inline esp_err_t usc_driver_ping(usc_config_t *config) 
{
    return usc_driver_write(config, STRINGIFY(PING), sizeof(PING));
}

static __always_inline uint32_t parse_data(const uint8_t *const data) 
{
    // Combine 4 bytes in big-endian order into 32-bit value
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

// needs to be changed to use the queue
usc_status_t handle_serial_key(usc_config_t *config, const UBaseType_t i)
{
    if (usc_driver_request_password(config) != ESP_OK) { // Request serial key
        ESP_LOGE(TAG, "Failed to send serial");
        return DATA_SEND_ERROR; // doesn't need system interface
    }

    SERIAL_RECIEVE_DELAY();

    // 1 is added for the NULL terminator which is junk
    uint8_t *key = uart_read(config->uart_config.port, config->buffer, config->buffer_size, uart_queue[i], PASSWORD_PING_DELAY);
    if (key != NULL) {
        ESP_LOGI(TAG, "Serial key: %u %u %u %u", key[0], key[1], key[2], key[3]);

        if (SERIAL_KEY == parse_data(key)) { // originally SERIAL_KEY
            return CONNECTED;
        }
    }
    return TIME_OUT;
}

static __always_inline void maintain_connection(usc_config_t *config) 
{
    usc_driver_ping(config);
    SERIAL_RECIEVE_DELAY();
}

// needs to be finished
static usc_status_t process_data(const uart_port_t port, SerialDataQueueHandler data_queue, serial_buffer_t buffer, const UBaseType_t i)
{
    uint8_t *temp_data = uart_read(port, buffer, buf_SIZE, uart_queue[i], SERIAL_INPUT_DELAY);
    if (temp_data != NULL) {
        uint32_t data = parse_data(temp_data);
        dataStorageQueue_add(data_queue, data); // Add the data to the queue
        return DATA_RECEIVED;
    }
    return DATA_RECEIVE_ERROR;
}

// do not rename
void usc_driver_read_task(void *pvParameters)
{
    const UBaseType_t index = uxTaskPriorityGet(NULL) - TASK_PRIORITY_START; // gets priority ID of the task, more temportary
    struct usc_driver_t *driver = (struct usc_driver_t *)pvParameters; // make a copy inside the function
    SemaphoreHandle_t sync_signal = driver->sync_signal; // Get the mutex lock for the driver

    struct usc_driver_base_t *driver_base = &driver->driver_storage;
    struct usc_config_t *config = &driver_base->driver_setting;
    struct usc_task_manager_t *current_task_config = &driver_base->driver_tasks; // get the task configuration
    bool *active = &current_task_config->active; // Check if the driver has access
    // prioirty should give the id number minus the offset of the task
    ESP_LOGI(TASK_TAG, "Priority %u\n", (index + TASK_PRIORITY_START)); // Debugging line to check task name and priority`
    ESP_LOGI(TASK_TAG, "Task status: %d\n", *active);
    vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay

    #ifdef MICROUSC_UART_DEBUG
    if (uart_queue[index] == NULL) {
        ESP_LOGE(TAG, "Uart queue is null, [%u]", index);
    }
    #endif
    
    //#if (NEEDS_SERIAL_KEY == 1)
    while (1) {
        if (xSemaphoreTake(sync_signal, portMAX_DELAY) == pdTRUE) {
            if (*active && !config->has_access) {
                config->status = handle_serial_key(config, index); // Check if the serial key is valid
                if (config->status != CONNECTED) {
                    ESP_LOGW(TASK_TAG, "Serial key check failed, retrying...");  // Debugging line to check if the serial key check failed
                }
                else {
                    config->has_access = true;
                }
                // Wait for the task to be activated
                xSemaphoreGive(sync_signal); // finish this part
                vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
            }
            else {
                xSemaphoreGive(sync_signal); // finish this part
                break;
            }
        }
    }
    //#endif

    while (1) {
        if (xSemaphoreTake(sync_signal, portMAX_DELAY) == pdTRUE) {
            if (*active == false) {
                xSemaphoreGive(sync_signal); // Release the mutex lock
                break;
            }

            // maintain_connection(config); // Ping the driver to check connection
            config->status = process_data(config->uart_config.port, config->data, config->buffer, index); // Read and process incoming data

            ESP_LOGI(TASK_TAG, "Task %d is running\n", index); // Debugging line to check task name and priority
            xSemaphoreGive(sync_signal); // Release the mutex lock
            vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
        }
    }

    ESP_LOGI(TASK_TAG, "Task %s is terminating...\n", config->driver_name); // Debugging line to check task name and priority
    //ESP_LOGI("TASK", "Task %s terminated", config->driver_name);
    vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
    vTaskDelete(NULL); // Delete the task
}