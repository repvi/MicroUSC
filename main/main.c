#include "nvs_flash.h" // doesn't need to be included, recommended to have
#include "MicroUSC/system/MicroUSC-internal.h"
#include "MicroUSC/USCdriver.h"
#include "testing_driver.h"
#include "speed_test.h"
//#include "xt_asm_utils.h"
//#include "xtensa/config/tie-asm.h"
// git log
// git checkout [c50cad7fbea3ae70313ac72c68d59a8db20e8dc8]
// git commit -m "Change details"
// git branch 
// git checkout -b [name of branch]
// git push origin [name of branch]
// git pull origin master
// git status
// git switch [name of branch]           switches to another branch

// 115200 baud rate

// xtensa-esp-elf-objdump -D build/MicroUSC.elf > disassembly.tx
// xtensa-esp-elf-objdump -t build/MicroUSC.elf > symbols.txt

// idf.py -D CMAKE_VERBOSE_MAKEFILE=ON build
// xtensa-esp32-elf-gcc -S -o output.S example.c     
// xtensa-esp32-elf-objdump -t build/MicroUSC.elf | findstr "example_function"
/*
// Function that runs from IRAM (faster but limited space)
void IRAM_ATTR critical_timing_function(void) {
    // Time-critical code here
}

// shell:RecycleBinFolder

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

// xtensa-esp-elf-addr2line -e build/MicroUSC.elf 0x400d679c

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_MicroUSC_system();

    uart_config_t setting = STANDARD_UART_CONFIG; // only for debugging
    /*
    uart_port_config_t pins = {
        .tx = GPIO_NUM_17,
        .rx = GPIO_NUM_16,
        .port = UART_NUM_2
    };
    */
   
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    printf("Heap Free: %d, Largest Free Block: %d, Allocated Blocks: %d\n",
    info.total_free_bytes, info.largest_free_block, info.allocated_blocks);

    uart_port_config_t pins = {
        .port = UART_NUM_2, // make it to 1
        .rx = GPIO_NUM_16, // 17
        .tx = GPIO_NUM_17 // 18
    };

    // code should go after here
    
    usc_process_t driver_action = &system_task; // point to the function you created
    // function will configure driver_example
    // 0 is for the driver type, for now you can only use 0 and 1.
    // do not use the same number or it will not be configured

    // uncomment the line below to test the speed of the function
    CHECK_FUNCTION_SPEED_WITH_DEBUG(usc_driver_init("first driver", setting, pins, driver_action, 4086));

    //usc_print_driver_configurations();
    //usc_print_overdriver_configurations();
    
    /*
    uart_port_config_t pinss = {
        .tx = GPIO_NUM_4,
        .rx = GPIO_NUM_5,
        .port = UART_NUM_1
    };

    CHECK_FUNCTION_SPEED_WITH_DEBUG(usc_driver_init("second driver", setting, pinss, driver_action));
    */   

    set_microusc_system_code(USC_SYSTEM_LED_ON);
    set_microusc_system_code(USC_SYSTEM_SPECIFICATIONS);
    set_microusc_system_code(USC_SYSTEM_DRIVER_STATUS);
    
    //printf("Pausing system...\n");
    //set_microusc_system_code(USC_SYSTEM_PAUSE);
    vTaskDelay(2000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
    set_microusc_system_code(USC_SYSTEM_LED_OFF);
    //vTaskDelay(4000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
    //set_microusc_system_code(USC_SYSTEM_RESUME);
    //vTaskDelay(2000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
    //set_microusc_system_code(USC_SYSTEM_ERROR);
    //set_microusc_system_code(USC_SYSTEM_SLEEP);

    printf("End of program\n");
}