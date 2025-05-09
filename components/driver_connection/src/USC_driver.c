#include <string.h>
#include "esp_system.h"
#include "USC_driver.h"
#include "esp_log.h"
#include "memory_pool.h"
#include "esp_heap_caps.h"
#include "esp_assert.h"

#include "speed_test.h"

#include <stdatomic.h>

//static memory_pool_t memory_serial_node = {}; // mandatory
usc_driver_t drivers[DRIVER_MAX] = {0}; // System drivers for the USC driver

// __attribute__((aligned(ESP32_ARCHITECTURE_ALIGNMENT_VAR)))
atomic_uint_least32_t serial_data[DRIVER_MAX] ESP32_ALIGNMENT = {0};  // Indexed access [1][3]

usc_driver_t overdrivers[OVERDRIVER_MAX] = {0};

/*
uint32_t usc_driver_get_data(const int i) {
    return queue_top(&drivers[i].config->data);
}
*/

// delete in the future
//static portMUX_TYPE queueLock = portMUX_INITIALIZER_UNLOCKED; // example of a mutex lock

// needs to be finsihed
void usc_driver_clean_data(const int i) {
    usc_driver_t *driver = &drivers[i]; // Get the driver configuration
    xSemaphoreTake(driver->sync_signal, portMAX_DELAY); // Wait for the mutex lock
    queue_clean(&driver->driver_setting.data);
    xSemaphoreGive(driver->sync_signal); // Release the mutex lock
}

void usc_driver_read_task(void *pvParameters); // header

static void create_usc_driver_task( usc_driver_t *driver, 
                                    const UBaseType_t i) { // might neeed adjustment
    const UBaseType_t DRIVER_TASK_Priority_START = TASK_PRIORITY_START + i;

    xTaskCreatePinnedToCore(
        usc_driver_read_task,                 // Task function
        driver->driver_setting.driver_name,   // Task name
        TASK_STACK_SIZE,                      // Stack size
        (void *)driver,                       // Task parameters
        DRIVER_TASK_Priority_START,           // Increment priority for next task
        &driver->driver_tasks.task_handle,    // Task handle
        TASK_CORE_READER                      // Core to pin the task
    );
}

// todo -> needs to be changed to have remove the task driver (reader) action of the uart
static void create_usc_driver_action_task( usc_data_process_t driver_process,
                                           const UBaseType_t i) {
    const UBaseType_t OFFSET = TASK_PRIORITY_START + i;
    char task_name[15];
    snprintf(task_name, sizeof(task_name), "Action #%d", i);

    xTaskCreatePinnedToCore(
        driver_process,                         // Task function
        task_name,                              // Task name
        TASK_STACK_SIZE,                        // Stack size
        (void *)i,                              // Task parameters
        (OFFSET),                               // Based on index
        &drivers[i].driver_tasks.action_handle, // Task handle
        TASK_CORE_ACTION                        // Core to pin the task
    );
}

// Validate UART configuration
static esp_err_t check_valid_uart_config( const uart_config_t uart_config,    
                                          const uart_port_config_t port_config,
                                          const UBaseType_t i) {
    if (i >= DRIVER_MAX) {
        ESP_LOGE("USB_DRIVER", "Invalid driver index");
        return ESP_ERR_INVALID_ARG;
    }

    if (OUTSIDE_SCOPE(port_config.port, UART_NUM_MAX - 1)) {
        ESP_LOGE("USB_DRIVER", "Invalid UART port");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize UART and memory pool
    QueueHandle_t queueH = uart_queue[i]; // Queue for UART data (it is a pointer)
    queueH = xQueueCreate(UART_QUEUE_SIZE, sizeof(char)); // Create the queue for UART data
    if (developer_input(uart_init(port_config, uart_config, queueH) != ESP_OK)) {
        ESP_LOGE("USB_DRIVER", "Initialization failed");
        vQueueDelete(queueH); // Delete the queue if initialization fails
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}


static void configure_driver_setting( usc_config_t *config,
                                      const uart_config_t uart_config,
                                      const uart_port_config_t port_config) {
    
    if (strncmp(config->driver_name, "", DRIVER_NAME_SIZE) == 0) {
        static int no_name = 1;
        snprintf(config->driver_name, DRIVER_NAME_SIZE, "Unknown Driver %d", no_name);
        config->driver_name[DRIVER_NAME_SIZE - 1] = '\0';
        no_name++;
    }

    config->has_access = false; // default to not have access
    // Set status and create the task
    config->status = NOT_CONNECTED; // As default
    config->baud_rate = uart_config.baud_rate; // in case, recommended to set
    config->uart_config = port_config;
}

static void configure_full_driver_setting(  usc_config_t *driver_setting,
                                     const UBaseType_t i) {
    usc_driver_t *driver = &drivers[i]; // Get the driver configuration
    driver->driver_setting = *driver_setting; // Get the driver configuration
    driver->driver_tasks.active = true; // Set the task as active
}

esp_err_t usc_driver_init( usc_config_t *config,
                           const uart_config_t uart_config,
                           const uart_port_config_t port_config,
                           usc_data_process_t driver_process,
                           const UBaseType_t i) {

    esp_err_t ret = check_valid_uart_config(uart_config, port_config, i);
    if (ret != ESP_OK) {
        ESP_LOGE("USB_DRIVER", "Invalid UART configuration");
        return ret;
    }

    usc_driver_t *driver = &drivers[i]; // Get the driver configuration
    if (driver->sync_signal != NULL) { // make sure to initzialize in the kernel
        ESP_LOGE("USB_DRIVER", "Driver already initialized");
        return ESP_ERR_INVALID_STATE; // Driver already initialized
    }

    configure_driver_setting(config, uart_config, port_config);
    while (!xSemaphoreTake(driver->sync_signal, SEMAPHORE_DELAY)); // Wait for the mutex lock
    configure_full_driver_setting(config, i); // Set the driver settings
    create_usc_driver_task(&drivers[i], i); // create the task for the serial driver implemented
    create_usc_driver_action_task(driver_process, i); // create task for the custom function (driver)
    xSemaphoreGive(driver->sync_signal); // Release the mutex lock

    printf("Index %d\n", i); // Debugging line to check task name and priority
    return ESP_OK;
}

esp_err_t usc_driver_write( const usc_config_t *config,
                            const char *data,
                            const size_t len) {
    if (config->baud_rate < CONFIGURED_BAUDRATE) { 
        literate_bytes(data, const char, len) {
            if (uart_write_bytes(config->uart_config.port, begin /* from literate_bytes */, 1) == -1) {
                return ESP_FAIL;
            }
            // some delay to allow the data to be sent
        }
        return ESP_OK;
    }
    return (uart_write_bytes(config->uart_config.port, data, len) == -1) ? ESP_FAIL : ESP_OK;
}

esp_err_t usc_driver_request_password(usc_config_t *config) {
    return usc_driver_write(config, REQUEST_KEY, sizeof(REQUEST_KEY));
}

esp_err_t usc_driver_ping(usc_config_t *config) {
    return usc_driver_write(config, PING, sizeof(PING));
}

// needs to be changed to use the queue
static bool handle_serial_key(usc_config_t *config) {
    usc_driver_request_password(config); // Request serial key
    vTaskDelay(SERIAL_REQUEST_DELAY_MS / portTICK_PERIOD_MS); // Wait for response
    printf("Waiting for serial key...\n"); // Debugging line to check if the task is running
    char *key = NULL; //uart_read(&config->uart_config.port, sizeof(SERIAL_KEY) + 1);
    if (key != NULL) {
        printf("Serial key: %s\n", key); // Debugging line to check if the task is running
        if (strcmp(key, SERIAL_KEY) == 0) {
            config->has_access = true;
            config->status = CONNECTED;
            heap_caps_free(key);
            return true;
        }
        heap_caps_free(key);
    }
    config->status = ERROR;
    return false;
}

static void maintain_connection(usc_config_t *config) {
    usc_driver_ping(config);
    vTaskDelay(SERIAL_REQUEST_DELAY_MS / portTICK_PERIOD_MS); // Allow time for the ping
}

static inline uint32_t parse_data(uint8_t *data) {
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

// needs to be finished
static void process_data(usc_config_t *config) {
    uint8_t *temp_data = NULL; //uart_read(&config->uart_config.port, SERIAL_REQUEST_SIZE + 1);
    if (temp_data != NULL) {
        config->status = DATA_RECEIVED;
        uint32_t data = parse_data(temp_data);
        queue_add(&config->data, data); // Add the data to the queue
        heap_caps_free(temp_data);
    }
    else {
        config->status = DATA_RECEIVE_ERROR;
    }
}

void usc_driver_read_task(void *pvParameters) {
    usc_config_t *config = (usc_config_t *)pvParameters;
    const UBaseType_t priority = uxTaskPriorityGet(NULL) - TASK_PRIORITY_START; // gets priority ID of the task, more temportary
    SemaphoreHandle_t *sync_signal = &drivers[priority].sync_signal; // Get the mutex lock for the driver
    usc_task_manager_t *current_task_config = &drivers[priority].driver_tasks; // get the task configuration
    bool *active = &current_task_config->active; // Check if the driver has access
    // prioirty should give the id number minus the offset of the task
    ESP_LOGI("TASK", "%s with priority %u\n", config->driver_name, priority); // Debugging line to check task name and priority`
    ESP_LOGI("TASK", "Task status: %d\n", current_task_config->active);

    #if (NEEDS_SERIAL_KEY == 1)
    while (1) {
        if (xSemaphoreTake(*sync_signal, SEMAPHORE_DELAY) == pdTRUE) {
            if ((*active) && !config->has_access) {
                if (!handle_serial_key(config)) {
                    printf("Serial key check failed, retrying...\n"); // Debugging line to check if the serial key check failed
                }
                // Wait for the task to be activated
                vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
            }
            else {
                break;
            }

            xSemaphoreGive(*sync_signal); // finish this part
        }
    }
    #endif

    while (1) {
        if (xSemaphoreTake(*sync_signal, SEMAPHORE_DELAY) == pdTRUE) {
            if (*active == false) {
                xSemaphoreGive(*sync_signal); // Release the mutex lock
                break;
            }

             //maintain_connection(config);     // Ping the driver to check connection
            process_data(config);              // Read and process incoming data

            ESP_LOGI("TASK", "Task %s is running with priority %u\n", config->driver_name, priority); // Debugging line to check task name and priority
            ESP_LOGI("TASK", "Task %s is running\n", config->driver_name); // Debugging line to check task name and priority
            vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
            xSemaphoreGive(*sync_signal); // Release the mutex lock
        }
    }

    ESP_LOGI("TASK", "Task %s is terminating...\n", config->driver_name); // Debugging line to check task name and priority
    //ESP_LOGI("TASK", "Task %s terminated", config->driver_name);
    //vTaskDelete(current_task_config->action_handle); // Delete the action task
    vTaskDelete(NULL); // Delete the task
}

esp_err_t usc_set_overdrive( const usc_config_t *config, 
                             usc_event_cb_t action, 
                             const int i) {
    if (OUTSIDE_SCOPE(i, OVERDRIVER_MAX) || action == NULL) {
        ESP_LOGE("USB_DRIVER", "Invalid overdriver");
        return ESP_ERR_INVALID_ARG;
    }

    while (xSemaphoreTake(overdrivers[i].sync_signal, SEMAPHORE_DELAY) != pdTRUE); // Wait for the mutex lock
    overdrivers[i].driver_setting = *config; // Get the driver configuration
    overdriver_action[i] = action; // Set the driver action callback
    xSemaphoreGive(overdrivers[i].sync_signal); // Release the mutex lock
    return ESP_OK;
}

esp_err_t overdrive_usc_driver( usc_driver_t *driver,
                                const int i) {
    if (OUTSIDE_SCOPE(i, OVERDRIVER_MAX)) {
        return ESP_FAIL;
    }

    usc_driver_t *overdriver = &overdrivers[i]; // Get the overdriver configuration
    while (xSemaphoreTake(overdriver->sync_signal, SEMAPHORE_DELAY) != pdTRUE); // Wait for the mutex lock
    if (overdriver->driver_tasks.action_handle == NULL) {
        xSemaphoreGive(overdriver->sync_signal); // Release the mutex lock
        return ESP_ERR_INVALID_STATE; // overdriver not initialized
    }

    xSemaphoreGive(overdriver->sync_signal); // Release the mutex lock

    while (xSemaphoreTake(driver->sync_signal, SEMAPHORE_DELAY) != pdTRUE); // Wait for the mutex lock
    driver->driver_setting = overdriver->driver_setting; // Get the driver configuration
    // implement the driver action callback
    xSemaphoreGive(driver->sync_signal); // Release the mutex lock
    return ESP_OK;
}