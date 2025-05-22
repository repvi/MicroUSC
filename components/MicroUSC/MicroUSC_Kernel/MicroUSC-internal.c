#include "MicroUSC-internal.h"
#include "MicroUSC-kernel.h"
#include "USCdriver.h"
#include "status.h"

/* n = 0  -> default value
   n = 1  -> turn off esp32
   n = 2  -> sleep mode
   n = 3  -> pause port data
   n = 4  -> connect to wifi
   n = 5  -> connect to bluetooth
   n = 6  -> turn on built-in LED on esp32
   n = 7  -> turn off built-in led on esp32
   n = 8  -> memory usage
   n = 9  -> system specs
   n = 10 -> driver status
*/
#define TAG "[MICROUSC KERNEL]"

#define BLINK_GPIO GPIO_NUM_2
#define RTC_MEMORY_BUFFER_SIZE (256) // Size of the memory buffer
#define RTC_MEMORY_STORAGE_KEY_SIZE (32) // Size of the key for the memory storage

#define TASK_STACK_SIZE (2048) // Stack size for the system task

RTC_NOINIT_ATTR unsigned int __system_reboot_count = 0;
    
RTC_NOINIT_ATTR struct rtc_memory_t {
    uint8_t buf[RTC_MEMORY_BUFFER_SIZE]; // RTC memory buffer
    unsigned int remaining_size; // Remaining size of the buffer
    char address_key[RTC_MEMORY_STORAGE_KEY_SIZE]; // Address of the initialized memory
    int address_key_index; // Index for the address key
} __rtc_memory = {.buf = {0}, .remaining_size = RTC_MEMORY_BUFFER_SIZE, .address_key = {0}, .address_key_index = 0};

microusc_status __microusc_system_code;

microusc_error_handler __microusc_system_error_handler;

void save_system_rtc_var(void *var, const size_t size, const char key) 
{
    if (var == NULL || size == 0 || key == '\0') {
        ESP_LOGE(TAG, "Invalid parameters for saving RTC variable");
        return;
    }

    if (size <= __rtc_memory.remaining_size && __rtc_memory.address_key_index < RTC_MEMORY_STORAGE_KEY_SIZE) {
        memcpy(__rtc_memory.buf, var, size); // Copy the variable to the RTC memory
        __rtc_memory.remaining_size -= size; // Decrease the remaining size
        __rtc_memory.address_key[__rtc_memory.address_key_index++] = key; // Save the key
        ESP_LOGI(TAG, "Saved %u bytes to RTC memory", size);
    }
    else{
        ESP_LOGE(TAG, "Not enough space in RTC memory");
        return;
    }
}

void *get_system_rtc_var(const char key)
{
    for (char *begin = __rtc_memory.address_key; *begin != '\0'; begin++) {
        if (*begin == key) {
            ESP_LOGI(TAG, "Key found in RTC memory");
            return __rtc_memory.buf;
        }
    }
    if (strncmp(__rtc_memory.address_key, &key, sizeof(char)) == 0) {
        return __rtc_memory.buf;
    }
    ESP_LOGE(TAG, "Key mismatch for RTC memory");
    return NULL;
}

void __microusc_system_error_handler_default(void)
{
    ESP_LOGE(TAG, "System error handler called");
    ESP_LOGE(TAG, "Rebooting system...");
    // shutdown all drivers function here
    usc_print_driver_configurations(); // Print the driver configurations
    __system_reboot_count++;
    esp_restart();
}

void set_microusc_system_error_handler(microusc_error_handler handler)
{
    if (handler == NULL) {
        ESP_LOGE(TAG, "Error handler is NULL");
        return;
    }
    __microusc_system_error_handler = handler;
}

void set_microusc_system_error_handler_default(void) 
{
    set_microusc_system_error_handler(__microusc_system_error_handler_default);
}

void set_microusc_system_code(microusc_status code)
{
    __microusc_system_code = code;
}

void __microusc_system_task(void *p)
{
    while (1) {
         switch(__microusc_system_code) {
            case USC_SYSTEM_OFF:
                esp_restart();
                break;
            case USC_SYSTEM_SLEEP:
                break;
            case USC_SYSTEM_PAUSE:

                break;
            case USC_SYSTEM_WIFI_CONNECT:

                break;
            case USC_SYSTEM_BLUETOOTH_CONNECT:

                break;
            case USC_SYSTEM_LED_ON:
                gpio_set_level(BLINK_GPIO, 1); // ON
                break;
            case USC_SYSTEM_LED_OFF:
                gpio_set_level(BLINK_GPIO, 0); // OFF
                break;
            case USC_SYSTEM_MEMORY_USAGE:

                break;
            case USC_SYSTEM_SPECIFICATIONS:

                break;
            case USC_SYSTEM_DRIVER_STATUS:

                break;
            case USC_SYSTEM_ERROR:
                __microusc_system_error_handler();
                break;
            default:
                __microusc_system_code = USC_SYSTEM_DEFAULT;
                break;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS); // 1 second delay
    }
}

esp_err_t microusc_system_task(void)
{
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    xTaskCreatePinnedToCore(
        __microusc_system_task,
        "microUSB System",
        TASK_STACK_SIZE,
        NULL,
        MICROUSC_SYSTEM_PRIORITY,
        NULL,
        MICROUSC_CORE
    );

    if (__system_reboot_count != 0) {
        ESP_LOGI(TAG, "System fail count: %u", __system_reboot_count);
    }

    set_microusc_system_error_handler_default(); // Set the default error handler
    set_microusc_system_code(USC_SYSTEM_DEFAULT); // Set the default system code
    return ESP_OK;
}