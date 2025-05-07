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

usc_stored_config_t drivers[DRIVER_MAX] = {{0}};

// __attribute__((aligned(ESP32_ARCHITECTURE_ALIGNMENT_VAR)))
atomic_uint_least32_t serial_data[DRIVER_MAX] ESP32_ALIGNMENT = {{0}};  // Indexed access [1][3]

usc_task_manager_t driver_task_manager[DRIVER_MAX] = {{0}}; // Task manager for the drivers

usc_stored_config_t overdrivers[OVERDRIVER_MAX] = {{0}};

uint32_t usb_driver_get_data(const int i) {
    return queue_top(&drivers[i].config->data);
}

// delete in the future
//static portMUX_TYPE queueLock = portMUX_INITIALIZER_UNLOCKED; // example of a mutex lock

// needs to be finsihed
void usb_driver_clean_data(const int i) {
    usc_stored_config_t *driver = &drivers[i]; // Get the driver configuration
    portENTER_CRITICAL(&driver->critical_lock); // Lock the queue
    queue_clean(&driver->config->data);
    portEXIT_CRITICAL(&driver->critical_lock); // Unlock the queue
}

esp_err_t usc_driver_deinit_all_tasks(void) { // new to change tasks
    cycle_usc_tasks() {
        if (usc_task->active) {
            usc_task->active = false; // Reset the active flag
            vTaskDelay(pdMS_TO_TICKS(DELAY_MILISECOND_50));  // Delay for 1 second
            
            usc_task->task_handle = NULL; // Reset the task handle
            usc_task->action_handle = NULL; // Reset the action task handle
        }
    }

    cycle_drivers() {
        driver->config = NULL; // Reset the driver configuration
        driver->driver_action = NULL; // Reset the driver action callback
    }

    return ESP_OK;
}

esp_err_t usc_overdriver_deinit_all(void) {
    cycle_overdrivers() {
        overdriver->config = NULL;
        overdriver->driver_action = NULL;
    }
    return ESP_OK;
}

static void check_driver_name(char *name) {
    if (strncmp(name, "", DRIVER_NAME_SIZE) == 0) {
        static int no_name = 1;
        snprintf(name, DRIVER_NAME_SIZE, "Unknown Driver %d", no_name);
        name[DRIVER_NAME_SIZE - 1] = '\0';
        no_name++;
    }
}

void usc_driver_read_task(void *pvParameters); // header

static void create_usc_driver_task( usc_config_t *config, 
                                    const UBaseType_t i) { // might neeed adjustment
    const UBaseType_t DRIVER_TASK_Priority_START = TASK_PRIORITY_START + i;

    xTaskCreatePinnedToCore(
        usc_driver_read_task,                // Task function
        config->driver_name,                 // Task name
        TASK_STACK_SIZE,                     // Stack size
        (void *)config,                      // Task parameters
        DRIVER_TASK_Priority_START,          // Increment priority for next task
        &driver_task_manager[i].task_handle, // Task handle
        TASK_CORE                            // Core to pin the task
    );
}

static void create_usc_driver_action_task( usc_data_process_t driver_process, 
                                           const UBaseType_t i) {
    const UBaseType_t OFFSET = TASK_PRIORITY_START + DRIVER_MAX + i;
    char task_name[15];
    snprintf(task_name, sizeof(task_name), "Action #%d", i);

    xTaskCreatePinnedToCore(
        driver_process,                        // Task function
        task_name,                             // Task name
        TASK_STACK_SIZE,                       // Stack size
        (void *)i,                             // Task parameters
        (OFFSET),                              // Based on index
        &driver_task_manager[i].action_handle, // Task handle
        TASK_CORE                              // Core to pin the task
    );
}

esp_err_t usc_driver_init( usc_config_t *config,
                           const uart_config_t uart_config,
                           const uart_port_config_t port_config,
                           usc_data_process_t driver_process,
                           const UBaseType_t i,
                           QueueHandle_t uart_queue) {
    // Validate UART configuration
    if (i >= DRIVER_MAX) {
        ESP_LOGE("USB_DRIVER", "Invalid driver index");
        return ESP_ERR_INVALID_ARG;
    }

    if (OUTSIDE_SCOPE(config->uart_config.port, UART_NUM_MAX - 1)) {
        ESP_LOGE("USB_DRIVER", "Invalid UART port");
        return ESP_ERR_INVALID_ARG;
    }

    config->uart_config = port_config;
    // Initialize UART and memory pool
    if (developer_input(uart_init(config->uart_config, uart_config, uart_queue) != ESP_OK)) {
        ESP_LOGE("USB_DRIVER", "Initialization failed");
        return ESP_FAIL;
    }

    // Initialize driver name
    check_driver_name(config->driver_name);

    config->has_access = false; // default to not have access
    // Set status and create the task
    config->status = NOT_CONNECTED; // As default
    config->baud_rate = uart_config.baud_rate; // in case, recommended to set

    usc_stored_config_t *driver = &drivers[i]; // Get the driver configuration
    driver->config = config; // store the configuration of the implemented serial driver
    driver->driver_action = driver_process; // Set the driver action callback
    driver->critical_lock = ( portMUX_TYPE )portMUX_INITIALIZER_UNLOCKED; // Initialize the mutex lock

    driver_task_manager[i].active = true; // Set the task as active
    create_usc_driver_task(config, i); // create the task for the serial driver implemented
    create_usc_driver_action_task(driver_process, i);
    
    printf("Index %d\n", i); // Debugging line to check task name and priority
    return ESP_OK;
}

esp_err_t usc_driver_write( const usc_config_t *config,
                            const char *data,
                            const size_t len) {
    if (config->baud_rate < CONFIGURED_BAUDRATE) { 
        literate_bytes(data, const char, len) {
            if (uart_write_bytes(config->uart_config.port, begin, 1) == -1) {
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
    UBaseType_t priority =  uxTaskPriorityGet(NULL) - TASK_PRIORITY_START; // gets priority ID of the task, more temportary
    usc_task_manager_t *current_task_config = &driver_task_manager[priority]; // get the task configuration
    bool *active = &current_task_config->active; // Check if the driver has access
    // prioirty should give the id number minus the offset of the task
    ESP_LOGI("TASK", "%s with priority %u\n", config->driver_name, priority); // Debugging line to check task name and priority`
    ESP_LOGI("TASK", "Task status: %d\n", current_task_config->active);

    #if (NEEDS_SERIAL_KEY == 1)
    while ((*active) && !config->has_access) {
        if (!handle_serial_key(config)) {
            printf("Serial key check failed, retrying...\n"); // Debugging line to check if the serial key check failed
        }
        // Wait for the task to be activated
        vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
    }
    #endif


    while (*active) {
        //maintain_connection(config);     // Ping the driver to check connection
        process_data(config);            // Read and process incoming data

        ESP_LOGI("TASK", "Task %s is running with priority %u\n", config->driver_name, priority); // Debugging line to check task name and priority
        ESP_LOGI("TASK", "Task %s is running\n", config->driver_name); // Debugging line to check task name and priority
        vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
    }

    ESP_LOGI("TASK", "Task %s is terminating...\n", config->driver_name); // Debugging line to check task name and priority
    //ESP_LOGI("TASK", "Task %s terminated", config->driver_name);
    //vTaskDelete(current_task_config->action_handle); // Delete the action task
    vTaskDelete(NULL); // Delete the task
}

esp_err_t usc_driver_deinit(serial_input_driver_t *driver) {
    memset(driver->config.driver_name, '\0', DRIVER_NAME_SIZE);

    uart_port_config_deinit(&driver->config.uart_config);

    driver->config.baud_rate = 0;
    driver->config.has_access = false;
    driver->config.status = NOT_CONNECTED;
    driver->driver_action = NULL;
    return ESP_OK;
}

esp_err_t usc_set_overdrive( const usc_config_t *config, 
                             const usc_event_cb_t action, 
                             const int i) {
    if (OUTSIDE_SCOPE(i, OVERDRIVER_MAX) || action == NULL) {
        ESP_LOGE("USB_DRIVER", "Invalid overdriver");
        return ESP_ERR_INVALID_ARG;
    }

    overdrivers[i].config = config; // store the configuration of the implemented serial driver
    overdrivers[i].driver_action = action; // Set the driver action callback

    return ESP_OK;
}

esp_err_t overdrive_usc_driver( serial_input_driver_t *driver, 
                                const int i) {
    if (OUTSIDE_SCOPE(i, OVERDRIVER_MAX) && overdrivers[i].driver_action == NULL) {
        return ESP_FAIL;
    }

    uart_port_config_t uart_config = overdrivers[i].config->uart_config;
    driver->config = *overdrivers[i].config;
    driver->config.uart_config = uart_config;
    driver->driver_action = overdrivers[i].driver_action;
    return ESP_OK;
}