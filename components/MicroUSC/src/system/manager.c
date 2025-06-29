#include "MicroUSC/internal/system/init.h"
#include "MicroUSC/internal/system/service_def.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/internal/driverList.h"
#include "MicroUSC/chip_specific/system_attr.h"
#include "MicroUSC/system/manager.h"
#include "MicroUSC/system/status.h"
#include "MicroUSC/USCdriver.h"
#include "esp_debug_helpers.h"
#include "esp_system.h"
#include <esp_task_wdt.h>
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

#define INTERNAL_TASK_STACK_SIZE (4096) // Stack size for the system task, increase the size in the future for development

#define WDT_TIMER_WAIT 3

#define WDT_TIMER_DELAY pdMS_TO_TICKS(1)

#define microusc_system_operation(topic, status, func, key, data) do { \
    send_to_mqtt_service_single(topic, key, data); \
    builtin_led_system(status); \
    func;  \
} while(0)

#define microusc_system_operation_quick(topic, func, key, data) do { \
    send_to_mqtt_service_single(topic, key, data); \
    func; \
} while(0)

#define microusc_pause_drivers() usc_drivers_pause()
#define microusc_resume_drivers() usc_drivers_resume()
#define microusc_queue_flush() xQueueReset(microusc_system.queue_system.queue_handler);

#define microusc_system_mqtt_main(topic, status, func, key, data) microusc_system_operation(topic, status, func, key, data)

#define microusc_system_mqtt_main_fast(topic, func, key, data) microusc_system_operation_quick(topic, func, key, data)

struct {
    struct {
        StackType_t stack[TASK_STACK_SIZE];
        StaticTask_t taskBuffer;
        TaskHandle_t main_task;
    } task;
    struct {
        QueueHandle_t queue_handler;
        size_t count;
    } queue_system;
    struct {
        microusc_error_handler operation;
        void *stored_var;
        int size;
    } error_handler;
    portMUX_TYPE critical_lock;
} microusc_system;

#define microusc_quick_context(des, val) do { \
    portENTER_CRITICAL(&microusc_system.critical_lock); \
    des = val; \
    portEXIT_CRITICAL(&microusc_system.critical_lock); \
} while(0)

void IRAM_ATTR microusc_software_isr_handler(void *arg)
{
    MiscrouscBackTrack_t current;
    current.status = *(microusc_status *)arg;
    xQueueSendFromISR(microusc_system.queue_system.queue_handler, &current, NULL);
}

/* Add on the next version */
__deprecated void microusc_system_isr_trigger(void) 
{
    /* TODO */
}

void microusc_system_isr_pin(gpio_config_t io_config, microusc_status trigger_status)
{
    int bit_mask = io_config.pin_bit_mask;
    int gpio_pin = 1;

    /* Determine the GPIO pin number from the bit mask */
    if (bit_mask == 0) return;
    while (bit_mask > 1) {
        bit_mask >>= 1;
        gpio_pin++;
    }

    /* Remove any existing ISR handler for this pin */
    gpio_isr_handler_remove((gpio_num_t)gpio_pin);

    /* Configure the GPIO pin with the provided settings */
    gpio_config(&io_config);

    /* Add the new ISR handler for this pin */
    gpio_isr_handler_add((gpio_num_t)gpio_pin, microusc_software_isr_handler, &gpio_pin);
}

void microusc_start_wifi(char *const ssid, char *const password)
{
    wifi_init_sta(ssid, password);
}

esp_err_t microusc_system_start_mqtt_service(esp_mqtt_client_config_t *mqtt_cfg)
{
    return init_mqtt(mqtt_cfg);
}

__attribute__((noreturn)) void microusc_system_restart(void)
{
    esp_restart();
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

    backtrack->type.caller_pc = (uint32_t)frame.pc;
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
        send_microusc_system_status(USC_SYSTEM_ERROR);
    }
    #endif
    microusc_system.error_handler.operation = handler;
    microusc_system.error_handler.stored_var = var;
    microusc_system.error_handler.size = size;
}

void set_microusc_system_error_handler_default(void)
{
    set_microusc_system_error_handler(microusc_system_error_handler_default, NULL, 0);
}

void send_microusc_system_status(microusc_status code)
{
    /* this only runs if the task is not suspended due to flushing the queue at the meantime */
    if (eTaskGetState(microusc_system.task.main_task) != eSuspended) {
        MiscrouscBackTrack_t data;
        data.status = code;

        if (code == USC_SYSTEM_ERROR || code == USC_SYSTEM_PRINT_SUCCUSS) {
            getBackPCprevious(&data, 1);
        }

        if (uxQueueSpacesAvailable(microusc_system.queue_system.queue_handler) != 0) {
            taskENTER_CRITICAL(&microusc_system.critical_lock);
            {
                microusc_system.queue_system.count = 0;
            }
            taskEXIT_CRITICAL(&microusc_system.critical_lock);
            xQueueSend(microusc_system.queue_system.queue_handler, &data, 0);
        }
        else {
            ESP_LOGW(TAG, "MicroUSC system queuehandler has overflowed");
            taskENTER_CRITICAL(&microusc_system.critical_lock);
            {
                microusc_system.queue_system.count++;
                if (microusc_system.queue_system.count == 3) {
                    microusc_queue_flush();
                }
            }
            taskEXIT_CRITICAL(&microusc_system.critical_lock);
        }
    }
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

static void microusc_system_task(void *p)
{
    MiscrouscBackTrack_t sys_data;
    while (1) {
        if (xQueueReceive(microusc_system.queue_system.queue_handler, &sys_data, portMAX_DELAY) == pdPASS) {
            printf("Called microUSC system\n");
            switch(sys_data.status) {
                case USC_SYSTEM_OFF:
                    microusc_system_restart();
                    break;
                case USC_SYSTEM_SLEEP:
                    builtin_led_system(USC_SYSTEM_SLEEP);
                    sleep_mode();
                    break;
                case USC_SYSTEM_PAUSE:
                    microusc_system_mqtt_main(CONNECTION_MQTT_SEND_INFO, sys_data.status, microusc_pause_drivers(), "status", "pause");
                    break;
                case USC_SYSTEM_RESUME:
                    microusc_system_mqtt_main(CONNECTION_MQTT_SEND_INFO, sys_data.status, microusc_resume_drivers(), "status", "normal");
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
                    microusc_system_mqtt_main_fast(CONNECTION_MQTT_SEND_INFO, call_usc_error_handler(sys_data.type.caller_pc), "status", "error");
                    break;
                default:
                    break;
            }
        }
    }
}

__attribute__((noreturn)) void microusc_infloop(void)
{
    while (1) {
        // nothing
    }
}

static esp_err_t microusc_system_setup(void)
{
    gpio_install_isr_service(0);
    microusc_system.critical_lock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

    microusc_system.queue_system.queue_handler = xQueueCreate(MICROUSC_QUEUEHANDLE_SIZE, sizeof(MiscrouscBackTrack_t));
    if (microusc_system.queue_system.queue_handler == NULL) {
        return ESP_ERR_NO_MEM;
    }
    microusc_system.queue_system.count = 0;

    #ifdef BUILTIN_LED_ASSEMBLY
    init_builtin_led();
    #endif
    
    set_rtc_cycle();

    sleep_mode_wakeup_default();
    set_microusc_system_error_handler_default(); /* Set the default error handler */

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

void init_MicroUSC_system(void) 
{
    ESP_ERROR_CHECK(init_system_memory_space()); /* Initialize memory pools for the system */
    ESP_ERROR_CHECK(microusc_system_setup()); /* system task will run on core 0, mandatory */
    vTaskDelay(500 / portTICK_PERIOD_MS); /* Wait for the system to be ready (500 milliseconds) */
}