#include "MicroUSC/system/rtc.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/internal/uscdef.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#define TAG "[RTC MEMORY]"

#define RTC_MEMORY_BUFFER_SIZE (256) // Size of the memory buffer
#define RTC_MEMORY_STORAGE_KEY_SIZE (32) // Size of the key for the memory storage

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
    uintptr_t address = 0; // 'NULL' equivalent
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