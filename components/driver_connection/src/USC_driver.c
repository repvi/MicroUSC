#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
#include "esp_system.h"
#include "USC_driver.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_assert.h"

#define TASK_PRIORITY_START             (10) // Used for the serial communication task
#define TASK_STACK_SIZE               (2048) // Used for saving in the heap for FREERTOS
#define TASK_CORE                        (1) // Core 1 is used for all other operations other than wifi or any wireless protocols

#define SERIAL_REQUEST_DELAY_MS        (30)
#define SERIAL_KEY_RETRY_DELAY_MS      (50)
#define LOOP_DELAY_MS                  (10)

#define OVERDRIVER_MAX                  (3)

#define MEMORY_BLOCK_MAX               (20)

#define DRIVER_NAME_SIZE              (sizeof(driver_name_t))

// save a lot of memory
typedef struct {
    usc_config_t *config;                    ///< Array of USB configurations
    usc_data_process_t driver_action;        ///< Action callback for the overdrive driver
} usc_stored_config_t;

static usc_stored_config_t drivers[DRIVER_MAX];
static usc_stored_config_t overdrivers[OVERDRIVER_MAX];

static memory_pool_t memory_serial_node;

void queue_add(Queue *queue, const char *restrict data) {
    struct QueueNode *new_block = (struct QueueNode *)memory_pool_alloc(&memory_serial_node);
    if (new_block == NULL) {
        return; // not allocated
    }
    
    strncpy(new_block->data, data, sizeof(queue->head->data));

    if (queue->head != NULL) {
        queue->tail->next = new_block;
        queue->tail = queue->tail->next;
    }
    else {
        queue->head = new_block;
        queue->tail = queue->head;
    }

    queue->count++;
}

void queue_remove(Queue *queue) {
    if (queue->head != NULL) {
        void *temp = (void *)queue->head;
        queue->head = queue->head->next;
        memory_pool_free(&memory_serial_node, temp);
        queue->count--;
    }
}

char *queue_top(Queue *queue) {
    if (queue->head != NULL) {
        char *data = queue->head->data; // get pointer to data
        queue_remove(queue);
        return data;
    }
    else {
        return NULL;
    }
}

void queue_delete(Queue *queue) {
    while (queue->head != NULL) {
        queue_remove(queue);   
    }
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

static void create_usc_driver_task(usc_config_t *config) {
    static int driver_TASK_Priority_START = TASK_PRIORITY_START;
    if (strcmp(config->driver_name, "") == 0) {
        static int num = 1;
        sprintf(config->driver_name, "Unknown Driver %d", num);
        num++;
    }

    xTaskCreatePinnedToCore(
        usc_driver_read_task,         // Task function
        config->driver_name,          // Task name
        TASK_STACK_SIZE,              // Stack size
        (void *)config,               // Task parameters
        driver_TASK_Priority_START++, // Increment priority for next task
        NULL,                         // Task handle
        TASK_CORE                     // Core to pin the task
    );
}

static void create_usc_driver_action_task(usc_data_process_t driver_process, const int i) {
    const int OFFSET = TASK_PRIORITY_START + DRIVER_MAX;

    char task_name[15];
    sprintf(task_name, "Action #%d", i);

    xTaskCreatePinnedToCore(
        driver_process,               // Task function
        task_name,                    // Task name
        TASK_STACK_SIZE,              // Stack size
        (void *)i,                    // Task parameters
        (OFFSET + i),                 // Based on index
        NULL,                         // Task handle
        TASK_CORE                     // Core to pin the task
    );
}

esp_err_t usc_driver_init(usc_config_t *config, uart_config_t uart_config, uart_port_config_t port_config, usc_data_process_t driver_process, int i) {
    // Validate UART configuration
    if (OUTSIDE_SCOPE(i, DRIVER_MAX)) {
        ESP_LOGE("USB_DRIVER", "Invalid driver index");
        return ESP_ERR_INVALID_ARG;
    }

    if (OUTSIDE_SCOPE(config->uart_config.port, UART_NUM_MAX - 1)) {
        ESP_LOGE("USB_DRIVER", "Invalid UART port");
        return ESP_ERR_INVALID_ARG;
    }

    config->uart_config = port_config;
    // Initialize UART and memory pool
    if (developer_input(uart_init(config->uart_config, uart_config) != ESP_OK)) {
        ESP_LOGE("USB_DRIVER", "Initialization failed");
        return ESP_FAIL;
    }

    if (!memory_serial_node.memory) {
        if (unlikely(memory_pool_init(&memory_serial_node, SIZE_OF_SERIAL_MEMORY_BLOCK, MEMORY_BLOCK_MAX) != ESP_OK)) {
            return ESP_ERR_NO_MEM;
        }
    }

    // Initialize driver name
    check_driver_name(config->driver_name);

    config->has_access = false; // default to not have access
    // Set status and create the task
    config->status = NOT_CONNECTED; // As default
    config->baud_rate = uart_config.baud_rate; // in case, recommended to set
    drivers[i].config = config; // store the configuration of the implemented serial driver
    drivers[i].driver_action = driver_process; // Set the driver action callback

    create_usc_driver_task(config); // create the task for the serial driver implemented
    create_usc_driver_action_task(driver_process, i);
    return ESP_OK;
}

void usc_print_driver_configurations(void) {
    for (int i = 0; i < DRIVER_MAX; i++) {
        if (drivers[i].driver_action == NULL) {
            ESP_LOGI("DRIVER", "NOT INITIALIZED on index %d", i);
            ESP_LOGI("      ", "----------------------------");
        }
        else {
            ESP_LOGI("DRIVER", " %s", drivers[i].config->driver_name);
            ESP_LOGI("Baud Rate", " %d", drivers[i].config->baud_rate);
            ESP_LOGI("Status", " %d", drivers[i].config->status);
            ESP_LOGI("Has Access", " %d", drivers[i].config->has_access);
            ESP_LOGI("UART Port", " %d", drivers[i].config->uart_config.port);
            ESP_LOGI("UART TX Pin", " %d", drivers[i].config->uart_config.tx);
            ESP_LOGI("UART RX Pin", " %d", drivers[i].config->uart_config.rx);
            ESP_LOGI("      ", "----------------------------");
        }
    }
}

void usc_print_overdriver_configurations(void) {
    for (int i = 0; i < OVERDRIVER_MAX; i++) {
        if (overdrivers[i].driver_action == NULL) {
            ESP_LOGI("OVERDRIVER", " NOT INITIALIZED on index %d", i);
            ESP_LOGI("          ", "----------------------------");
        }
        else {
            ESP_LOGI("OVERDRIVER", " %s", overdrivers[i].config->driver_name);
            ESP_LOGI("Baud Rate", " %d", overdrivers[i].config->baud_rate);
            ESP_LOGI("          ", "----------------------------");
        }
    }
}

esp_err_t usc_driver_write(const usc_config_t *config, const char *data, const size_t len) {
    if (config->baud_rate < CONFIGURED_BAUDRATE) { 
        for (int i = 0; i < len; i++) {
            if (uart_write_bytes(config->uart_config.port, &data[i], 1) == -1) {
                return ESP_FAIL;
            }
        }
        return ESP_OK;
    }
    else {
        return (uart_write_bytes(config->uart_config.port, data, len) == -1) ? ESP_FAIL : ESP_OK;
    }
}

esp_err_t usc_driver_request_password(usc_config_t *config) {
    return usc_driver_write(config, REQUEST_KEY, sizeof(REQUEST_KEY));
}

esp_err_t usc_driver_ping(usc_config_t *config) {
    return usc_driver_write(config, PING, sizeof(PING));
}

static bool handle_serial_key(usc_config_t *config) {
    usc_driver_request_password(config); // Request serial key
    vTaskDelay(SERIAL_REQUEST_DELAY_MS / portTICK_PERIOD_MS); // Wait for response

    char *key = uart_read(&config->uart_config.port, sizeof(serial_key_t));
    if (key != NULL) {
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

static void process_data(usc_config_t *config) {
    char *temp_data = uart_read(&config->uart_config.port, sizeof(serial_data_t));
    if (temp_data != NULL) {
        config->status = DATA_RECEIVED;
        queue_add(&config->data, temp_data);
        heap_caps_free(temp_data);
    }
    else {
        config->status = DATA_RECEIVE_ERROR;
    }
}

void usc_driver_read_task(void *pvParameters) {
    usc_config_t *config = (usc_config_t *)pvParameters;

    while (1) {
        if (!config->has_access) {
            if (!handle_serial_key(config)) {
                continue;                 // Retry if serial key check fails
            }
        }

        maintain_connection(config);     // Ping the driver to check connection
        process_data(config);            // Read and process incoming data

        vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
    }
}

static void clear_serial_memory(Queue *serial_memory) { // not used yet
    queue_delete(serial_memory);
}

esp_err_t usc_driver_deinit(serial_input_driver_t *driver) {
    memset(driver->config.driver_name, '\0', DRIVER_NAME_SIZE);

    uart_port_config_deinit(&driver->config.uart_config);

    driver->config.baud_rate = 0;
    driver->config.has_access = false;
    driver->config.status = NOT_CONNECTED;
    //clear_serial_memory(&driver->config.data); // not neccessary
    driver->driver_action = NULL;
    return ESP_OK;
}

static esp_err_t manage_overdrivers(usc_stored_config_t *overdriver, usc_config_t *config, usc_event_cb_t action) {
    if (action == NULL) {
        return ESP_FAIL;
    }

    overdriver->driver_action = action;
    overdriver->config = config;

    return ESP_OK;
}

esp_err_t usc_set_overdrive(usc_config_t *config, usc_event_cb_t action, int i) {
    if (OUTSIDE_SCOPE(i, OVERDRIVER_MAX)) {
        ESP_LOGE("USB_DRIVER", "Invalid overdriver index");
        return false;
    }
    return manage_overdrivers(&overdrivers[i], config, action);
}

esp_err_t overdrive_usc_driver(serial_input_driver_t *driver, int i) {
    if (OUTSIDE_SCOPE(i, OVERDRIVER_MAX) && overdrivers[i].driver_action == NULL) {
        return ESP_FAIL;
    }

    uart_port_config_t uart_config = overdrivers[i].config->uart_config;
    driver->config = *overdrivers[i].config;
    driver->config.uart_config = uart_config;
    driver->driver_action = overdrivers[i].driver_action;
    return ESP_OK;
}

// completed, do not change
void usc_overdriver_deinit_all(void) {
    for (int i = 0; i < OVERDRIVER_MAX; i++) {
        overdrivers[i].config = NULL;
        overdrivers[i].driver_action = NULL;
    }
}