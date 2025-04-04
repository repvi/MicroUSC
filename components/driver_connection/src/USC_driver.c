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

const int DRIVER_NAME_SIZE = sizeof(driver_name_t);
/*
// Function that runs from IRAM (faster but limited space)
void IRAM_ATTR critical_timing_function(void) {
    // Time-critical code here
}

// Data that persists across deep sleep
RTC_DATA_ATTR uint32_t boot_count = 0;

// Data in RTC slow memory (persists in deep sleep, slower access)
RTC_SLOW_ATTR uint8_t slow_memory_buffer[512];

// Data in RTC fast memory (persists in light sleep, faster access)
RTC_FAST_ATTR uint8_t fast_memory_buffer[128];

// Function that should be executed from flash (saves IRAM)
IRAM_ATTR void normal_function(void) {
    // Non-time-critical code
}

// Data that must be accessible during cache disabled periods
DRAM_ATTR uint32_t cache_disabled_buffer[64];
*/

// save a lot of memory
typedef struct {
    usc_config_t *config;                    ///< Array of USB configurations
    usc_data_process_t driver_action;        ///< Action callback for the overdrive driver
} usc_stored_config_t;

static usc_stored_config_t drivers[DRIVER_MAX];
static usc_stored_config_t overdrivers[OVERDRIVER_MAX];

memory_pool_t memory_serial_node;

static esp_err_t initialize_uart(usc_config_t *config, uart_config_t uart_config, const uart_port_config_t *port_config) {
    esp_err_t ret = uart_init(config->uart_config, uart_config);
    if (ret != ESP_OK) {
        config->status = ERROR;
        config->uart_config = *port_config;
    }
    return ret;
}

static void initialize_driver_name(usc_config_t *config) {
    if (strncmp(config->driver_name, "", DRIVER_NAME_SIZE) == 0) {
        static int no_name = 1;
        snprintf(config->driver_name, DRIVER_NAME_SIZE, "Unknown Driver %d", no_name);
        config->driver_name[DRIVER_NAME_SIZE - 1] = '\0';
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
        driver_TASK_Priority_START++,       // Increment priority for next task
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

    if (OUTSIDE_SCOPE(config->uart_config.port, UART_NUM_MAX)) {
        ESP_LOGE("USB_DRIVER", "Invalid UART port");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize UART and memory pool
    if (developer_input(initialize_uart(config, uart_config, &port_config) != ESP_OK)) {
        ESP_LOGE("USB_DRIVER", "Initialization failed");
        return ESP_FAIL;
    }

    if (!memory_serial_node.memory) {
        if (!unlikely(memory_pool_init(&memory_serial_node, SIZE_OF_SERIAL_MEMORY_BLOCK, MEMORY_BLOCK_MAX) != ESP_OK)) {
            return ESP_ERR_NO_MEM;
        }
    }

    // Initialize driver name
    initialize_driver_name(config);

    config->has_access = false; // default to not have access
    // Set status and create the task
    config->status = NOT_CONNECTED; // As default
    config->baud_rate = uart_config.baud_rate; // in case, recommended to set
    drivers[i].config = config; // store the configuration of the implemented serial driver
    drivers[i].driver_action = driver_process; // Set the driver action callback

    create_usc_driver_task(config); // create the task for the serial driver implemented
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

esp_err_t usc_driver_write(usc_config_t *config, const serial_data_ptr_t data, size_t len) {
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

    serial_data_ptr_t key = uart_read(&config->uart_config.port, sizeof(serial_key_t));
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

// todo - Implement where to store the grabbed serial data in the system
static void process_data(usc_config_t *config) {
    serial_data_ptr_t temp_data = uart_read(&config->uart_config.port, sizeof(serial_data_t));
    if (temp_data != NULL) {
        config->status = DATA_RECEIVED;
        queue_add(&memory_serial_node, &config->data);
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
                continue; // Retry if serial key check fails
            }
        }

        maintain_connection(config); // Ping the driver to check connection
        process_data(config);        // Read and process incoming data

        vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); // 10ms delay
    }
}

void uart_port_config_deinit(uart_port_config_t *uart_config) {
    uart_config->port = UART_NUM_MAX; // Not a real PORT
    uart_config->rx = GPIO_NUM_NC; // -1
    uart_config->tx = GPIO_NUM_NC; // -1
}

static void clear_serial_memory(memory_pool_t *pool, Queue *serial_memory) {
    queue_delete(pool, serial_memory);
}

esp_err_t usc_driver_deinit(serial_input_driver_t *driver) {
    memset(driver->config.driver_name, '\0', DRIVER_NAME_SIZE);

    uart_port_config_deinit(&driver->config.uart_config);

    driver->config.baud_rate = 0;
    driver->config.has_access = false;
    driver->config.status = NOT_CONNECTED;
    clear_serial_memory(&memory_serial_node, &driver->config.data);
    driver->driver_action = NULL;
    return ESP_OK;
}

static esp_err_t manage_overdrivers(usc_stored_config_t *overdriver, usc_config_t *config, usc_event_cb_t action) {
    if (action == NULL) {
        return ESP_FAIL;
    }

    overdriver->driver_action = action;
    overdriver->config = config;
    //overdriver->config->status = NOT_CONNECTED; unnessary

    uart_port_config_deinit(&overdriver->config->uart_config);
    return ESP_OK;
}

esp_err_t usc_set_overdrive(usc_config_t *config, usc_event_cb_t action, int i) {
    if (OUTSIDE_SCOPE(i, OVERDRIVER_MAX)) {
        ESP_LOGE("USB_DRIVER", "Invalid overdriver index");
        return false;
    }
    return manage_overdrivers(&overdrivers[i], config, action);
}

esp_err_t overdrive_usb_driver(serial_input_driver_t *driver, int i) {
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
