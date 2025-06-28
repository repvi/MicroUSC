#include "MicroUSC/internal/system/init.h"
#include "MicroUSC/internal/system/bit_manip.h"
#include "MicroUSC/internal/driverList.h"
#include "MicroUSC/internal/genList.h"

#define TAG "[MICROUSC KERNEL]"

RTC_NOINIT_ATTR unsigned int system_reboot_count; // only accessed by the system
RTC_NOINIT_ATTR unsigned int checksum; // only accessed by the system

__always_inline unsigned int calculate_checksum(unsigned int value)
{
    return value ^ 0xA5A5A5A5; // XOR-based checksum for simplicity
}

void set_rtc_cycle(void)
{
    if (checksum != calculate_checksum(system_reboot_count)) {
        system_reboot_count = 0; // Reset only on corruption detection
    } 
    else {
        system_reboot_count++; // Safe increment
    }
    
    checksum = calculate_checksum(system_reboot_count); // Update valid checksum

    if (system_reboot_count != 0) {
        ESP_LOGW(TAG, "System fail count: %u", system_reboot_count);
    }
}

void increment_rtc_cycle(void)
{
    set_rtc_cycle();
}

static esp_err_t init_memory_handlers(void)
{
    driver_system.lock = xSemaphoreCreateBinary(); /* initialize the mux (mandatory) */
    if (driver_system.lock == NULL) {
        ESP_LOGE(TAG, "Could not initialize main driver lock");
        return ESP_FAIL;
    }
    xSemaphoreGive(driver_system.lock);
    INIT_LIST_HEAD(&driver_system.driver_list.list);

    return init_hidden_driver_lists(sizeof(uint32_t) + 2 /* was SEND_BUFFER_SIZE */, 256);
}

esp_err_t init_system_memory_space(void) 
{
    esp_err_t ret = init_memory_handlers();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize memory handlers");
        return ret;
    }

    ret = init_configuration_storage();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize configuration storage");
        return ret;
    }

    return ret;
}