#include "MicroUSC/chip_specific/system_attr.h"
#include "MicroUSC/system/MicroUSC-internal.h"
#include "MicroUSC/system/kernel.h"
#include "MicroUSC/system/status.h"
#include "MicroUSC/synced_driver/USCdriver.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/internal/initStart.h"
#include "MicroUSC/internal/genList.h"
#include "esp_system.h"
#include <esp_task_wdt.h>
#include "esp_chip_info.h"
#include "esp_sleep.h"
#include "esp_intr_alloc.h"
#include "esp_attr.h"

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

#define RTC_MEMORY_BUFFER_SIZE (256) // Size of the memory buffer
#define RTC_MEMORY_STORAGE_KEY_SIZE (32) // Size of the key for the memory storage

#define CONVERT_TO_SLEEPMODE_TIME(x) ( uint64_t ) ( pdMS_TO_TICKS(x) * portTICK_PERIOD_MS * 1000ULL )
#define DEFAULT_LIGHTMODE_TIME CONVERT_TO_SLEEPMODE_TIME(5000) // 5 seconds

#define INTERNAL_TASK_STACK_SIZE (4096) // Stack size for the system task, increase the size in the future for development

#define WDT_TIMER_WAIT 3
#define WDT_TIMER_DELAY pdMS_TO_TICKS(1)
RTC_NOINIT_ATTR unsigned int __system_reboot_count;
    
RTC_NOINIT_ATTR struct rtc_memory_t {
    uint8_t buf[RTC_MEMORY_BUFFER_SIZE]; // RTC memory buffer
    unsigned int remaining_size; // Remaining size of the buffer
    char address_key[RTC_MEMORY_STORAGE_KEY_SIZE]; // Address of the initialized memory
    int address_key_index; // Index for the address key
} __rtc_memory = {.buf = {0}, .remaining_size = RTC_MEMORY_BUFFER_SIZE, .address_key = {0}, .address_key_index = 0};

struct sleep_config_t {
    gpio_num_t wakeup_pin;
    uint64_t sleep_time;
    bool wakeup_pin_enable;
    bool sleep_time_enable;
} __microusc_system_sleep;

microusc_error_handler __microusc_system_error_handler;

RTC_NOINIT_ATTR unsigned int checksum;

intr_handle_t micro_usc_isr_handler = NULL;

QueueHandle_t __microusc_queue_action = NULL;

void IRAM_ATTR microusc_software_isr_handler(void *arg)
{
    /*
    ESP_LOGI(TAG, "Called the isr");
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(__microusc_lock, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    //esp_intr_disable(micro_usc_isr_handler);
    */
}

__deprecated void __system_isr_trigger(void) 
{
    printf("Triggering from core %d\n", xPortGetCoreID());
    esp_intr_enable(micro_usc_isr_handler);
}

__deprecated void __system_task_trigger(void) 
{
    /*
    xTaskNotifyGive(__microusc_critical_handle);
    */
}

__deprecated void microusc_system_isr_pin(gpio_num_t pin) 
{
    static gpio_num_t previous_isr_pin = GPIO_NUM_NC;
    if (previous_isr_pin != GPIO_NUM_NC) { 
        gpio_isr_handler_remove(pin); // completely gets removed, maybe change in the future
                
        if (pin != GPIO_NUM_NC) {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << pin),  // Example: GPIO0
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = GPIO_PULLUP_ENABLE,
                .intr_type = GPIO_INTR_NEGEDGE  // Or your desired edge
            };
            
            gpio_config(&io_conf);
            gpio_isr_handler_add(pin, microusc_software_isr_handler, NULL);
        }
    }

    previous_isr_pin = pin;
}

__always_inline void microusc_set_sleep_mode_timer_wakeup(uint64_t time) 
{
    __microusc_system_sleep.sleep_time = time;
}

__always_inline void microusc_set_sleep_mode_timer(bool option) 
{
    __microusc_system_sleep.sleep_time_enable = option;
}

__always_inline void microusc_set_wakeup_pin(gpio_num_t pin) 
{
    __microusc_system_sleep.wakeup_pin = pin;
}

__always_inline void microusc_set_wakeup_pin_status(bool option)
{
    __microusc_system_sleep.wakeup_pin_enable = option;
}

void microusc_set_sleepmode_wakeup_default(void) 
{
    microusc_set_sleep_mode_timer_wakeup(DEFAULT_LIGHTMODE_TIME);
    microusc_set_sleep_mode_timer(true);
    microusc_set_wakeup_pin(GPIO_NUM_NC);
    microusc_set_wakeup_pin_status(false);
}

static __noreturn void microusc_system_restart(void)
{
    esp_restart();
}

static __always_inline unsigned int calculate_checksum(unsigned int value)
{
    return value ^ 0xA5A5A5A5; // XOR-based checksum for simplicity
}

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

static void set_rtc_cycle(void)
{
    if (checksum != calculate_checksum(__system_reboot_count)) {
        __system_reboot_count = 0; // Reset only on corruption detection
    } 
    else {
        __system_reboot_count++; // Safe increment
    }
    
    checksum = calculate_checksum(__system_reboot_count); // Update valid checksum
}

static __always_inline void __increment_rtc_cycle(void) 
{
    set_rtc_cycle();
}

void __microusc_sleep_mode(void)
{
    bool sleep_time_enabled = __microusc_system_sleep.sleep_time_enable;
    bool wakeup_pin_enable = __microusc_system_sleep.wakeup_pin_enable;

    if (!(sleep_time_enabled || wakeup_pin_enable)) {
        return; // do nothing as timer is completely disabled
    }
    if (sleep_time_enabled) {
        esp_sleep_enable_timer_wakeup(__microusc_system_sleep.sleep_time);
    }
    if (wakeup_pin_enable) {
        esp_sleep_enable_ext0_wakeup(__microusc_system_sleep.wakeup_pin, 1);
    }

    esp_deep_sleep_start();
}

void __microusc_system_error_handler_default(void)
{
    ESP_LOGE(TAG, "System error handler called");
    ESP_LOGE(TAG, "Rebooting system...");
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second) experimental
    // shutdown all drivers function here
    usc_print_driver_configurations(); // Print the driver configurations
    __increment_rtc_cycle(); // Increment the RTC cycle variable
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

inline void set_microusc_system_error_handler_default(void) 
{
    set_microusc_system_error_handler(__microusc_system_error_handler_default);
}

inline void set_microusc_system_code(microusc_status code)
{
    xQueueSend(__microusc_queue_action, &code, portMAX_DELAY);
}

void print_system_info(void) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("ESP32 Chip Info:\n");
    printf("  Model: %s\n", chip_info.model == CHIP_ESP32 ? "ESP32" : "Other");
    printf("  Cores: %d\n", chip_info.cores);
    printf("  Features: WiFi%s%s\n", 
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
}

inline void __call_usc_error_handler(void) 
{
    __microusc_system_error_handler();
}

void IRAM_ATTR microusc_pause_drivers(void)
{
    struct usc_driverList *current, *tmp;
    list_for_each_entry_safe(current, tmp, &driver_system.driver_list.list, list) { // might be unsafe
        struct usc_task_manager_t *task_manager = &current->driver.driver_storage.driver_tasks;
        vTaskSuspend(task_manager->task_handle);
        vTaskSuspend(task_manager->action_handle);
    }
}

void IRAM_ATTR microusc_resume_drivers(void)
{
    struct usc_driverList *current, *tmp;
    list_for_each_entry_safe(current, tmp, &driver_system.driver_list.list, list) { // might be unsafe
        struct usc_task_manager_t *task_manager = &current->driver.driver_storage.driver_tasks;
        vTaskResume(task_manager->task_handle);
        vTaskResume(task_manager->action_handle);
    }
}

static void microusc_system_task(void *p)
{
    microusc_status system_status;
    while (1) {
        if (xQueueReceive(__microusc_queue_action, &system_status, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "Internal has been called!");
            switch(system_status) {
                case USC_SYSTEM_OFF:
                    microusc_system_restart();
                    break;
                case USC_SYSTEM_SLEEP:
                    builtin_led_system(USC_SYSTEM_SLEEP);
                    __microusc_sleep_mode();
                    break;
                case USC_SYSTEM_PAUSE:
                    builtin_led_system(USC_SYSTEM_PAUSE);
                    microusc_pause_drivers();
                    break;
                case USC_SYSTEM_WIFI_CONNECT:
                    builtin_led_system(USC_SYSTEM_WIFI_CONNECT);
                    // conflicting feature
                    break;
                case USC_SYSTEM_BLUETOOTH_CONNECT:
                    builtin_led_system(USC_SYSTEM_BLUETOOTH_CONNECT);
                    // conflicting feature
                    break;
                case USC_SYSTEM_LED_ON:
                    ESP_LOGI(TAG, "Turning on led...");
                    builtin_led_system(USC_SYSTEM_LED_ON);
                    break;
                case USC_SYSTEM_LED_OFF:
                    ESP_LOGI(TAG, "Turning off led...");
                    builtin_led_system(USC_SYSTEM_LED_OFF);
                    break;
                case USC_SYSTEM_MEMORY_USAGE:
                    // implement in the future
                    break;
                case USC_SYSTEM_SPECIFICATIONS:
                    print_system_info();
                    break;
                case USC_SYSTEM_DRIVER_STATUS:
                    usc_print_driver_configurations();
                    break;
                case USC_SYSTEM_ERROR:
                    builtin_led_system(USC_SYSTEM_ERROR);
                    __call_usc_error_handler();
                    break;
                default:
                    break;
            }
        }
    }
}

// needs to implement gpio isr trigger
esp_err_t microusc_system_setup(void)
{
    __microusc_queue_action = xQueueCreate(3, sizeof(microusc_status));
    if (__microusc_queue_action == NULL) {
        return ESP_ERR_NO_MEM;
    }

    init_builtin_led();
    
    xTaskCreatePinnedToCore(
        microusc_system_task,
        "microUSB System",
        INTERNAL_TASK_STACK_SIZE,
        NULL,
        MICROUSC_SYSTEM_PRIORITY,
        NULL,
        MICROUSC_CORE
    );

    set_rtc_cycle(); // Set the RTC cycle variable

    if (__system_reboot_count != 0) {
        ESP_LOGW(TAG, "System fail count: %u", __system_reboot_count);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second) experimental
    }

    microusc_set_sleepmode_wakeup_default();

    set_microusc_system_error_handler_default(); // Set the default error handler
    set_microusc_system_code(USC_SYSTEM_DEFAULT); // Set the default system code
    set_microusc_system_code(USC_SYSTEM_DEFAULT); // Set the default system code
    set_microusc_system_code(USC_SYSTEM_DEFAULT); // Set the default system code

    ESP_LOGI(TAG, "Internal has been set");
    return ESP_OK;
}