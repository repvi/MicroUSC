#include "testing_driver.h"
#include "speed_test.h"
#include "USC_driver.h"
#include "nvs_flash.h" // doesn't need to be included, recommended to have
#include "testing_driver.h"
#include "tiny_kernel.h"

// git log
// git checkout [c50cad7fbea3ae70313ac72c68d59a8db20e8dc8]
// git commit -m "Change details"
// git branch 
// git checkout -b [name of branch]
// git push origin [name of branch]
// git pull origin master
// git status
// git switch [name of branchgit ]           switches to another branch

// 115200 baud rate

// xtensa-esp-elf-objdump -D build/ESP32_USC_DRIVERS.elf > disassembly.tx
// xtensa-esp-elf-objdump -t build/ESP32_USC_DRIVERS.elf > symbols.txt

// idf.py -D CMAKE_VERBOSE_MAKEFILE=ON build
// xtensa-esp32-elf-gcc -S -o output.S example.c     
// xtensa-esp32-elf-objdump -t build/ESP32_USC_DRIVERS.elf | findstr "example_function"
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

/*
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};
*/

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_tiny_kernel();

    uart_config_t setting = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE, // should be defined by sdkconfig
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_port_config_t pins = {
        .tx = GPIO_NUM_1,
        .rx = GPIO_NUM_3,
        .port = UART_NUM_0
    };

    // code should go after here

    // functions like init_[something([varaible type] &configuration);

    usc_config_t driver_example = {
        .driver_name = "Driver example"
    };
    
    usc_data_process_t driver_action = &system_task; // point to the function you created
    // function will configure driver_example
    // 0 is for the driver type, for now you can only use 0 and 1.
    // do not use the same number or it will not be configured

    //printf("Starting driver initialization...\n");
    // uncomment the line below to test the speed of the function
    CHECK_FUNCTION_SPEED_WITH_DEBUG(usc_driver_init(&driver_example, setting, pins, driver_action, 0), ret);
    printf("End of program\n");
}