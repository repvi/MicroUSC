#include "MicroUSC/internal/system/bit_manip.h"
#include "MicroUSC/chip_specific/system_attr.h"
#include "MicroUSC/system/MicroUSC-internal.h"
#include "MicroUSC/system/status.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/internal/driverList.h"
#include "MicroUSC/internal/genList.h"
#include "MicroUSC/internal/uscdef.h"
#include "MicroUSC/USCdriver.h"
#include "esp_debug_helpers.h"
#include "esp_system.h"
#include <esp_task_wdt.h>
#include "esp_chip_info.h"
#include "esp_sleep.h"
#include "esp_intr_alloc.h"
#include "esp_attr.h"
#include "stdint.h"
#include <inttypes.h>

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

RTC_NOINIT_ATTR unsigned int system_reboot_count; // only accessed by the system
RTC_NOINIT_ATTR unsigned int checksum; // only accessed by the system

struct rtc_map {
    char key;
    uint8_t size;
};

RTC_NOINIT_ATTR struct rtc_memory_t {
    struct rtc_map mapping[RTC_MEMORY_STORAGE_KEY_SIZE];
    uint8_t buf[RTC_MEMORY_BUFFER_SIZE]; // RTC memory buffer
    portMUX_TYPE lock;
    int address_key_index; // Index for the address key
    int remaining_mem;
} rtc_memory = {.mapping = {}, .buf = {0}, .lock = portMUX_INITIALIZER_UNLOCKED, .address_key_index = 0, .remaining_mem = RTC_MEMORY_BUFFER_SIZE};

#define INITIALIZE_RTC_MEMORY { \
    .mapping = {}, \
    .buf = {0}, \
    .lock = portMUX_INITIALIZER_UNLOCKED, \
    .address_key_index = 0, \
    .remaining_mem = RTC_MEMORY_BUFFER_SIZE \
}

struct sleep_config_t {
    gpio_num_t wakeup_pin;
    uint64_t sleep_time;
    bool wakeup_pin_enable;
    bool sleep_time_enable;
} microusc_system_sleep;

struct {
    struct {
        StackType_t stack[TASK_STACK_SIZE];
        StaticTask_t taskBuffer;
        TaskHandle_t main_task;
    } task;
    struct {
        gpio_num_t wakeup_pin;
        uint64_t sleep_time;
        bool wakeup_pin_enable;
        bool sleep_time_enable;
    } deep_sleep;
    struct {
        QueueHandle_t queue_handler;
        size_t count;
    } queue_system;
    portMUX_TYPE critical_lock;
    intr_handle_t isr_handler;
    struct {
        microusc_error_handler operation;
        void *stored_var;
        int size;
    } error_handler;
} microusc_system;

// can be used custom api
typedef struct {
    uint32_t caller_pc;
    microusc_status status;
} MiscrouscBackTrack_t;

#define microusc_quick_context(des, val) \
    portENTER_CRITICAL(&microusc_system.critical_lock); \
    des = val; \
    portEXIT_CRITICAL(&microusc_system.critical_lock);

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
    //printf("Triggering from core %d\n", xPortGetCoreID());
    //esp_intr_enable(micro_usc_isr_handler);
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
    microusc_quick_context(microusc_system.deep_sleep.sleep_time, time);
}

__always_inline void microusc_set_sleep_mode_timer(bool option) 
{
    microusc_quick_context(microusc_system.deep_sleep.sleep_time_enable, option);
}

__always_inline void microusc_set_wakeup_pin(gpio_num_t pin) 
{
    microusc_quick_context(microusc_system.deep_sleep.wakeup_pin, pin);
}

__always_inline void microusc_set_wakeup_pin_status(bool option)
{
    microusc_quick_context(microusc_system.deep_sleep.wakeup_pin_enable, option);
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

    portENTER_CRITICAL(&rtc_memory.lock);
    {
        if (size <= rtc_memory.remaining_mem && rtc_memory.address_key_index < RTC_MEMORY_STORAGE_KEY_SIZE) {
            const int offset = RTC_MEMORY_BUFFER_SIZE - rtc_memory.remaining_mem;
            uint8_t *ptr = rtc_memory.buf + offset;
            memcpy(ptr, var, size); // Copy the variable to the RTC memory
            rtc_memory.remaining_mem -= size; // Decrease the remaining size
            rtc_memory.mapping[rtc_memory.address_key_index++].key = key; // Save the key            
            ESP_LOGI(TAG, "Saved %u bytes to RTC memory", size);
        }
        else{
            ESP_LOGE(TAG, "Not enough space in RTC memory");
        }
    }
    portEXIT_CRITICAL(&rtc_memory.lock);
}

void *get_system_rtc_var(const char key)
{
    uintptr_t address = 0; // 'NULL'
    portENTER_CRITICAL(&rtc_memory.lock);
    {
        size_t offset = 0;
        define_iteration(rtc_memory.mapping, struct rtc_map, map, RTC_MEMORY_STORAGE_KEY_SIZE) {
            if (map->key == key) {
                address = (uintptr_t)rtc_memory.buf + offset;
                break;
            }
            offset += map->size;
        }
    }
    portEXIT_CRITICAL(&rtc_memory.lock);
    return (void *)address;
}

static void set_rtc_cycle(void)
{
    if (checksum != calculate_checksum(system_reboot_count)) {
        system_reboot_count = 0; // Reset only on corruption detection
    } 
    else {
        system_reboot_count++; // Safe increment
    }
    
    checksum = calculate_checksum(system_reboot_count); // Update valid checksum
}

static __always_inline void increment_rtc_cycle(void)
{
    set_rtc_cycle();
}

static void microusc_sleep_mode(void)
{
    portENTER_CRITICAL(&microusc_system.critical_lock);
    {
        bool sleep_time_enabled = microusc_system.deep_sleep.sleep_time_enable;
        bool wakeup_pin_enable = microusc_system.deep_sleep.wakeup_pin_enable;
        if (!(sleep_time_enabled || wakeup_pin_enable)) {
            return; // do nothing as timer is completely disabled
        }
        
        if (sleep_time_enabled) {
            esp_sleep_enable_timer_wakeup(microusc_system.deep_sleep.sleep_time);
        }
        if (wakeup_pin_enable) {
            esp_sleep_enable_ext0_wakeup(microusc_system.deep_sleep.wakeup_pin, 1);
        }
    }
    portEXIT_CRITICAL(&microusc_system.critical_lock);
    esp_deep_sleep_start();
}

void IRAM_ATTR microusc_pause_drivers(void)
{
    struct usc_driverList *current, *tmp;
    list_for_each_entry_safe(current, tmp, &driver_system.driver_list.list, list) { // might be unsafe
        struct usc_driver_t *driver = &current->driver;
        vTaskSuspend(driver->uart_processor.task);
        vTaskSuspend(driver->uart_reader.task);
    }
}

void IRAM_ATTR microusc_resume_drivers(void)
{
    struct usc_driverList *current, *tmp;
    list_for_each_entry_safe(current, tmp, &driver_system.driver_list.list, list) { // might be unsafe
        struct usc_driver_t *driver = &current->driver;
        vTaskResume(driver->uart_processor.task);
        vTaskResume(driver->uart_reader.task);
    }
}


static void getBackPCprevious(MiscrouscBackTrack_t *backtrack, const size_t amount) 
{
    esp_backtrace_frame_t frame;
    
    // Get the starting frame
    esp_backtrace_get_start(&frame.pc, &frame.sp, &frame.next_pc);
    // get the one that called this function
    for (int i = 0; i < amount && frame.next_pc != 0; i++) {
        esp_backtrace_get_next_frame(&frame);
    }

    backtrack->caller_pc = (uint32_t)frame.pc;
}

static void microusc_system_error_handler_default(void *var)
{
    ESP_LOGE(TAG, "System error handler called");
    ESP_LOGE(TAG, "Rebooting system...");
    // shutdown all drivers function here
    usc_print_driver_configurations(); // Print the driver configurations
    microusc_pause_drivers();
    fflush(stdout); // make sure everything gets printed out
    microusc_system_restart();
}

void set_microusc_system_error_handler(microusc_error_handler handler, void *var, int size)
{
    #ifdef MICROUSC_DEBUG
    if (handler == NULL) {
        set_microusc_system_code(USC_SYSTEM_ERROR);
    }
    #endif
    microusc_system.error_handler.operation = handler;
    microusc_system.error_handler.stored_var = var;
    microusc_system.error_handler.size = size;
}

__always_inline void set_microusc_system_error_handler_default(void)
{
    set_microusc_system_error_handler(microusc_system_error_handler_default, NULL, 0);
}

// completely flush the queue to make it fully available
static void microusc_queue_flush(void)
{
    xQueueReset(microusc_system.queue_system.queue_handler);
}

inline void set_microusc_system_code(microusc_status code)
{
    if (eTaskGetState(microusc_system.task.main_task) != eSuspended) {
        // this only runs if the task is not suspended due to flushing the queue at the meantime
        MiscrouscBackTrack_t backtrack;
        backtrack.status = code;

        if (code == USC_SYSTEM_ERROR || code == USC_SYSTEM_PRINT_SUCCUSS) {
            esp_backtrace_frame_t frame;
    
            // Get the starting frame
            esp_backtrace_get_start(&frame.pc, &frame.sp, &frame.next_pc);
            // get the one that called this function
            for (int i = 0; i < 1 && frame.next_pc != 0; i++) {
                esp_backtrace_get_next_frame(&frame);
            }

            backtrack.caller_pc = (uint32_t)frame.pc;
        }

        if (uxQueueSpacesAvailable(microusc_system.queue_system.queue_handler) != 0) {
            taskENTER_CRITICAL(&microusc_system.critical_lock);
            {
                microusc_system.queue_system.count = 0;
            }
            taskEXIT_CRITICAL(&microusc_system.critical_lock);
            xQueueSend(microusc_system.queue_system.queue_handler, &backtrack, 0);
        }
        else {
            ESP_LOGE(TAG, "MicroUSC system queuehandler has overflowed");
            taskENTER_CRITICAL(&microusc_system.critical_lock);
            {
                microusc_system.queue_system.count++;
                if (microusc_system.queue_system.count == 3) {
                    microusc_queue_flush();
                    // don't need to make count to 0 unless for safety
                }
            }
            taskEXIT_CRITICAL(&microusc_system.critical_lock);
        }
    }
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

static void call_usc_error_handler(uint32_t pc)
{
    microusc_error_handler func;
    void *tmp = 0;
    ESP_LOGE(TAG, "Called from two levels back: 0x%08" PRIx32, pc);
    portENTER_CRITICAL(&microusc_system.critical_lock); 
    {
        const int var_size = microusc_system.error_handler.size;
        func = microusc_system.error_handler.operation;

        if (var_size != 0) {
            tmp = heap_caps_malloc(var_size, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
            memcpy(tmp, microusc_system.error_handler.stored_var, var_size);
        }
    }
    portEXIT_CRITICAL(&microusc_system.critical_lock);
    func(tmp);

    if (tmp != NULL) {
        heap_caps_free(tmp);
    }
}

/**
 * @brief Display ESP32 memory usage statistics for DMA and internal memory regions
 * 
 * This function queries the ESP-IDF heap capabilities API to retrieve and display
 * memory usage information for DMA-capable and internal memory regions.
 * 
 * @warning If this function crashes with LoadProhibited exception, it indicates
 *          heap corruption has already occurred earlier in program execution.
 *          Common causes include:
 *          - Buffer overflows writing past allocated memory boundaries
 *          - Use-after-free operations accessing freed memory
 *          - Double-free operations corrupting heap metadata
 *          - Memory alignment issues causing improper memory access
 *          - Stack overflow damaging heap structures
 * 
 * @note Function uses local macro MEMORY_TAG for consistent logging output
 * @note All memory values are retrieved as const to prevent modification
 */
static void show_memory_usage(void) 
{
    // Local macro for consistent logging tag - scoped only to this function
    #define MEMORY_TAG "[MEMORY]"
    
    // Query DMA capable memory statistics
    // DMA memory is required for hardware DMA operations and is typically limited
    const size_t total_dma = heap_caps_get_total_size(MALLOC_CAP_DMA);
    const size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
    
    // Log DMA memory information
    ESP_LOGI(MEMORY_TAG, "DMA capable memory:");
    ESP_LOGI(MEMORY_TAG, "  Total: %d bytes", total_dma);
    ESP_LOGI(MEMORY_TAG, "  Free: %d bytes", free_dma);
    
    // Query internal SRAM memory statistics  
    // Internal memory is fast SRAM, preferred for performance-critical operations
    const size_t total_internal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    const size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    
    // Log internal memory information
    ESP_LOGI(MEMORY_TAG, "Internal memory:");
    ESP_LOGI(MEMORY_TAG, "  Total: %d bytes", total_internal);
    ESP_LOGI(MEMORY_TAG, "  Free: %d bytes", free_internal);
    
    // Macro cleanup - undefine to prevent scope leakage
    #undef MEMORY_TAG
}

static void microusc_system_task(void *p)
{
    MiscrouscBackTrack_t backtrack;
    while (1) {
        if (xQueueReceive(microusc_system.queue_system.queue_handler, &backtrack, portMAX_DELAY) == pdPASS) {
            printf("Called microUSC system\n");
            switch(backtrack.status) {
                case USC_SYSTEM_OFF:
                    microusc_system_restart();
                    break;
                case USC_SYSTEM_SLEEP:
                    builtin_led_system(USC_SYSTEM_SLEEP);
                    microusc_sleep_mode();
                    break;
                case USC_SYSTEM_PAUSE:
                    builtin_led_system(USC_SYSTEM_PAUSE);
                    microusc_pause_drivers();
                    break;
                case USC_SYSTEM_RESUME:
                    builtin_led_system(USC_SYSTEM_SUCCESS); // turns off the built-in led
                    microusc_resume_drivers();
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
                    show_memory_usage();
                    break;
                case USC_SYSTEM_SPECIFICATIONS:
                    print_system_info();
                    break;
                case USC_SYSTEM_DRIVER_STATUS:
                    usc_print_driver_configurations();
                    break;
                case USC_SYSTEM_ERROR:
                    builtin_led_system(USC_SYSTEM_ERROR);
                    call_usc_error_handler(backtrack.caller_pc);
                    break;
                default:
                    break;
            }
        }
    }
}

__noreturn void microusc_infloop(void)
{
    while (1) {
        // nothing
    }
}
// needs to implement gpio isr trigger
esp_err_t microusc_system_setup(void)
{
    microusc_system.critical_lock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

    microusc_system.queue_system.queue_handler = xQueueCreate(MICROUSC_QUEUEHANDLE_SIZE, sizeof(MiscrouscBackTrack_t));
    if (microusc_system.queue_system.queue_handler == NULL) {
        return ESP_ERR_NO_MEM;
    }
    microusc_system.queue_system.count = 0;

    init_builtin_led();

    set_rtc_cycle(); // Set the RTC cycle variable

    if (system_reboot_count != 0) {
        ESP_LOGW(TAG, "System fail count: %u", system_reboot_count);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second) experimental
    }

    microusc_set_sleepmode_wakeup_default();
    set_microusc_system_error_handler_default(); // Set the default error handler

    xTaskCreatePinnedToCore(
        microusc_system_task,
        "microUSC System",
        INTERNAL_TASK_STACK_SIZE,
        NULL,
        MICROUSC_SYSTEM_PRIORITY,
        &microusc_system.task.main_task,
        MICROUSC_CORE
    );

    return ESP_OK;
}

static esp_err_t init_memory_handlers(void) {
    driver_system.lock = xSemaphoreCreateBinary(); // initialize the mux (mandatory)
    if (driver_system.lock == NULL) {
        ESP_LOGE(TAG, "Could not initialize main driver lock");
        return ESP_FAIL;
    }
    xSemaphoreGive(driver_system.lock);
    INIT_LIST_HEAD(&driver_system.driver_list.list);

    return init_hidden_driver_lists(4, 256);
}

void init_MicroUSC_system(void) {
    ESP_ERROR_CHECK(init_memory_handlers());
    ESP_ERROR_CHECK(init_configuration_storage());
    ESP_ERROR_CHECK(microusc_system_setup()); // system task will run on core 0, mandatory
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
}