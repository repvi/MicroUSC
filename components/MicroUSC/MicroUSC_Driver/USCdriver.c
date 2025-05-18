#include "debugging/speed_test.h"
#include "string.h"
#include "uscdef.h"
#include "initStart.h"
#include "esp_system.h"
#include "USCdriver.h"
#include "esp_log.h"
#include "memory_pool.h"
#include "speed_test.h"
#include "stdatomic.h"
#include "esp_uart.h"

#define TAG                "[USC DRIVER]"
#define TASK_TAG           "[DRIVER READER]"

#define INITIALIZE_USC_BIT_MANIP { .active_driver_bits = 0x0 };

#define NOT_FOUND (( uint32_t ) ( -1 ) )

struct usc_bit_manip {
    uint32_t active_driver_bits;
    SemaphoreHandle_t lock;
};

struct usc_bit_manip priority_storage = INITIALIZE_USC_BIT_MANIP;

Queue serial_data[DRIVER_MAX] = {0};  // Indexed access [1][3]

esp_err_t init_priority_storage(void) {
    STATIC_INIT_SAFETY init = initZERO;
    if (init == initONE) {
        return ESP_FAIL; // doesn't allow for reinitialization
    }
    priority_storage.lock = xSemaphoreCreateBinary();
    if (priority_storage.lock == NULL) {
        return ESP_ERR_NO_MEM;
    }
    xSemaphoreGive(priority_storage.lock);
    init = initONE;
    return ESP_OK;
}

// needs to be finsihed
/*
void usc_driver_clean_data(usc_driver_t *driver) {
    xSemaphoreTake(driver->sync_signal, portMAX_DELAY); // Wait for the mutex lock
    queue_clean(&driver->driver_setting.data);
    xSemaphoreGive(driver->sync_signal); // Release the mutex lock
}
*/

//returns the first bit that is 0
uint32_t getCurrentEmptyDriverIndex(void) {
    // make sure the max is less than 32 bits regardless
    uint32_t *driver_bits = &priority_storage.active_driver_bits;
    SemaphoreHandle_t lock = priority_storage.lock;
    uint32_t v;
    for (uint32_t i = 0; i < DRIVER_MAX; i++) { // there can be alternate code for this function, could have performance difference between the two possibly
        xSemaphoreTake(lock, portMAX_DELAY);
        v = BIT(i);
        if ((v & *driver_bits) == 0) {
            *driver_bits |= v; // bit is now occupied
            xSemaphoreGive(lock);
            return i; // returns empty bit
        }
        xSemaphoreGive(lock);
    }
    return NOT_FOUND;
}

void usc_driver_read_task(void *pvParameters); // header

void create_usc_driver_task( struct usc_driver_base_t *driver,
                                    const UBaseType_t i) { // might neeed adjustment
    const UBaseType_t DRIVER_TASK_Priority_START = TASK_PRIORITY_START + i;

    xTaskCreatePinnedToCore(
        usc_driver_read_task,                      // Task function
        driver->driver_setting.driver_name,   // Task name
        TASK_STACK_SIZE,                           // Stack size
        (void *)driver,                       // Task parameters
        DRIVER_TASK_Priority_START,                // Increment priority for next task
        &driver->driver_tasks.task_handle,    // Task handle
        TASK_CORE_READER                           // Core to pin the task
    );
}

// todo -> needs to be changed to have remove the task driver (reader) action of the uart
void create_usc_driver_action_task( struct usc_driver_t *driver,
                                           usc_data_process_t driver_process,
                                           const UBaseType_t i) {
    const UBaseType_t OFFSET = TASK_PRIORITY_START + i;
    char task_name[15];
    snprintf(task_name, sizeof(task_name), "Action #%d", i);

    xTaskCreatePinnedToCore(
        driver_process,                              // Task function
        task_name,                                   // Task name
        TASK_STACK_SIZE,                             // Stack size
        (void *)i,                                   // Task parameters
        (OFFSET),                                    // Based on index
        &driver->driver_tasks.action_handle,    // Task handle
        TASK_CORE_ACTION                             // Core to pin the task
    );
}

// Validate UART configuration
esp_err_t check_valid_uart_config( const uart_config_t *uart_config,    
                                   const uart_port_config_t *port_config) 
{
    if ((driver_system.size + 1) >= DRIVER_MAX) {
        ESP_LOGE(TAG, "Invalid driver index");
        return ESP_ERR_INVALID_ARG;
    }

    if (OUTSIDE_SCOPE(port_config->port, UART_NUM_MAX - 1)) {
        ESP_LOGE(TAG, "Invalid UART port");
        return ESP_ERR_INVALID_ARG;
    }

    UBaseType_t i = getCurrentEmptyDriverIndex();

    ESP_LOGI(TAG, "Got index: %u", i); // show what index it is starting at
    vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay

    // Initialize UART and memory pool
    QueueHandle_t queueH = uart_queue[i]; // Queue for UART data (it is a pointer)
    queueH = xQueueCreate(UART_QUEUE_SIZE, sizeof(char)); // Create the queue for UART data
    if (developer_input(uart_init(*port_config, *uart_config, queueH) != ESP_OK)) {
        ESP_LOGE(TAG, "Initialization failed");
        vQueueDelete(queueH); // Delete the queue if initialization fails
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

struct usc_driver_t configure_driver_setting( const char *driver_name,
                                              const uart_config_t uart_config,
                                              const uart_port_config_t port_config) 
{
    struct usc_driver_t driver;
    struct usc_config_t *driver_setting = &driver.driver_setting;
    driver.sync_signal = xSemaphoreCreateBinary(); // probably add if it is NULL or not
    if (driver.sync_signal == NULL) {
        return (struct usc_driver_t){};
    }
    xSemaphoreGive(driver.sync_signal); // create the section
    driver.driver_tasks.active = true; // Set the task as active

    if (strncmp(driver_setting->driver_name, "", DRIVER_NAME_SIZE - 1) != 0) {
        strncpy(driver_setting->driver_name, driver_name, sizeof(driver_name_t) - 1); // maybe needs the -1
    }
    else {
        static int no_name = 1;
        snprintf(driver_setting->driver_name, DRIVER_NAME_SIZE, "Unknown Driver %d", no_name);
        no_name++;
    }
    driver_setting->driver_name[DRIVER_NAME_SIZE - 1] = '\0';
    
    driver_setting->has_access = false; // default to not have access
    // Set status and create the task
    driver_setting->status = NOT_CONNECTED; // As default
    driver_setting->baud_rate = uart_config.baud_rate; // in case, recommended to set
    driver_setting->uart_config = port_config;
    return driver;
}

struct usc_driver_t *getLastDriver(void) {
    struct usc_driverList *node = list_last_entry(&driver_system.driver_list.list, struct usc_driverList, list); // use the tail of the list (the one that has been recently added)
    return (node != NULL) ? &node->driver_storage : NULL;
}

esp_err_t usc_driver_init( const char *driver_name,
                           const uart_config_t uart_config,
                           const uart_port_config_t port_config,
                           usc_data_process_t driver_process) 
{
    esp_err_t ret = check_valid_uart_config(&uart_config, &port_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Invalid UART configuration");
        return ret;
    }

    CHECKPOINT_START;
    // crashes around here
    CHECKPOINT_MESSAGE;
    struct usc_driverList *node = (struct usc_driverList *)memory_pool_alloc(mem_block_driver_nodes);
    CHECKPOINT_MESSAGE;
    const uint32_t open_spot = getCurrentEmptyDriverIndex(); // retrieve the first empty bit
    CHECKPOINT_MESSAGE;
    node->driver_storage = configure_driver_setting(driver_name, uart_config, port_config);
    CHECKPOINT_MESSAGE;
    SemaphoreHandle_t system_lock = driver_system.lock;
    CHECKPOINT_MESSAGE;
    xSemaphoreTake(system_lock, portMAX_DELAY); // WILL NOT CONTINUE IF IT DOES NOT RECIEVE AUTHERIZATION 1 IN
    CHECKPOINT_MESSAGE;
    struct usc_driver_t *current_driver = getLastDriver();
    if (current_driver == NULL) {
        ESP_LOGE(TAG, "Failed to get the last driver in the system driver manager");
        return ESP_ERR_NO_MEM;
    }
    CHECKPOINT_MESSAGE;
    if (!xSemaphoreTake(current_driver->sync_signal, SEMAPHORE_WAIT_TIME)) { // Wait for the mutex lock 2IN
        ESP_LOGE(TAG, "Failed to take semaphore");
        xSemaphoreGive(system_lock); // 1 OUT
        return ESP_ERR_INVALID_STATE; // Failed to take semaphore
    }

    CHECKPOINT_MESSAGE;
    create_usc_driver_task(current_driver, open_spot); // create the task for the serial driver implemented
    CHECKPOINT_MESSAGE;
    create_usc_driver_action_task(current_driver, driver_process, open_spot); // create task for the custom function (driver)
    CHECKPOINT_MESSAGE;

    xSemaphoreGive(system_lock); // 1 OUT
    CHECKPOINT_MESSAGE;
    xSemaphoreGive(current_driver->sync_signal); // Release the mutex lock 2 OUT
    CHECKPOINT_MESSAGE;
    printf("Configuration has been set on: %lu\n", open_spot); // Debugging line to check task name and priority
    // to here where it does not currently run
    return ESP_OK;
}

esp_err_t usc_driver_write( const usc_config_t *config,
                            const char *data,
                            const size_t len) 
{
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

esp_err_t usc_driver_request_password(usc_config_t *config) 
{
    return usc_driver_write(config, REQUEST_KEY, sizeof(REQUEST_KEY));
}

esp_err_t usc_driver_ping(usc_config_t *config) 
{
    return usc_driver_write(config, PING, sizeof(PING));
}

// needs to be changed to use the queue
bool handle_serial_key(usc_config_t *config, const UBaseType_t i) 
{
    usc_driver_request_password(config); // Request serial key
    vTaskDelay(SERIAL_REQUEST_DELAY_MS / portTICK_PERIOD_MS); // Wait for response
    //uint8_t *key = uart_read(&config->uart_config.port, sizeof(SERIAL_KEY) + 1, uart_queue[i]);
    // const uint8_t HIDDEN_KEY = 0x34; // something
    /*
    if (key != NULL) {
        printf("Serial key: %s\n", key); // Debugging line to check if the task is running
        if (key == 0) {
            config->has_access = true;
            config->status = CONNECTED;
            heap_caps_free(key);
            return true;
        }
        heap_caps_free(key);
    }
    */
    config->status = ERROR;
    return false;
}

void maintain_connection(usc_config_t *config) 
{
    usc_driver_ping(config);
    vTaskDelay(SERIAL_REQUEST_DELAY_MS / portTICK_PERIOD_MS); // Allow time for the ping
}

inline uint32_t parse_data(uint8_t *data) 
{
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

// needs to be finished
void process_data(usc_config_t *config) 
{
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

void usc_driver_read_task(void *pvParameters) 
{
    const UBaseType_t index = uxTaskPriorityGet(NULL) - TASK_PRIORITY_START; // gets priority ID of the task, more temportary
    struct usc_driver_t *driver = (struct usc_driver_t *)pvParameters;
    struct usc_config_t *config = &driver->driver_setting;
    SemaphoreHandle_t sync_signal = driver->sync_signal; // Get the mutex lock for the driver
    struct usc_task_manager_t *current_task_config = &driver->driver_tasks; // get the task configuration
    bool *active = &current_task_config->active; // Check if the driver has access
    // prioirty should give the id number minus the offset of the task
    ESP_LOGI(TASK_TAG, "Priority %u\n", (index + TASK_PRIORITY_START)); // Debugging line to check task name and priority`
    ESP_LOGI(TASK_TAG, "Task status: %d\n", *active);
    
    //#if (NEEDS_SERIAL_KEY == 1)
    while (1) {
        if (xSemaphoreTake(sync_signal, SEMAPHORE_DELAY) == pdTRUE) {
            if (*active && !config->has_access) {
                if (!handle_serial_key(config, index)) {
                    ESP_LOGW(TASK_TAG, "Serial key check failed, retrying...");  // Debugging line to check if the serial key check failed
                }
                // Wait for the task to be activated
                vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
                xSemaphoreGive(sync_signal); // finish this part
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

             //maintain_connection(config); // Ping the driver to check connection
            process_data(config); // Read and process incoming data

            ESP_LOGI(TASK_TAG, "Task %d is running\n", index); // Debugging line to check task name and priority
            vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
            xSemaphoreGive(sync_signal); // Release the mutex lock
        }
    }

    ESP_LOGI(TASK_TAG, "Task %s is terminating...\n", config->driver_name); // Debugging line to check task name and priority
    //ESP_LOGI("TASK", "Task %s terminated", config->driver_name);
    //vTaskDelete(current_task_config->action_handle); // Delete the action task
    vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
    vTaskDelete(NULL); // Delete the task
}