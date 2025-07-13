#include "MicroUSC/system/status.h"
#include "MicroUSC/internal/driverList.h"
#include "MicroUSC/USCdriver.h"
#include "esp_chip_info.h"
#include "esp_system.h"

#define TAG "[STATUS]"
#define MEMORY_TAG "[MEMORY]"

// best to save SRAM for the esp32 which is essentail, log(n) time complexity
static char *status_str(usc_status_t status)
{
    switch (status) {
        case NOT_CONNECTED:         return "NOT_CONNECTED";
        case CONNECTED:             return "CONNECTED";
        case DISCONNECTED:          return "DISCONNECTED";
        case ERROR:                 return "ERROR";
        case DATA_RECEIVED:         return "DATA_RECEIVED";
        case DATA_SENT:             return "DATA_SENT";
        case DATA_SEND_ERROR:       return "DATA_SEND_ERROR";
        case DATA_RECEIVE_ERROR:    return "DATA_RECEIVE_ERROR";
        case DATA_SEND_TIMEOUT:     return "DATA_SEND_TIMEOUT";
        case DATA_RECEIVE_TIMEOUT:  return "DATA_RECEIVE_TIMEOUT";
        case DATA_SEND_COMPLETE:    return "DATA_SEND_COMPLETE";
        case DATA_RECEIVE_COMPLETE: return "DATA_RECEIVE_COMPLETE";
        case TIME_OUT:              return "TIME_OUT";
        default:                    return "UNKNOWN_STATUS";
    }
}

void usc_print_driver_configurations(void)
{
    int i = 0;
    struct usc_driverList *current, *tmp;
    list_for_each_entry_safe(current, tmp, &driver_system.driver_list.list, list) {
        struct usc_driver_t *driver = &current->driver;
        SemaphoreHandle_t lock = driver->sync_signal;
        if (xSemaphoreTake(lock, SEMAPHORE_WAIT_TIME) == pdTRUE) {
            printf("%s       %s", "DRIVER",  driver->driver_name);
            printf("%s     %d", "Baud Rate",  driver->uart_config.baud_rate);
            printf("%s        %s", "Status",  status_str(driver->status));
            printf("%s     %d", "UART Port",  driver->port_config.port);
            printf("%s   %d", "UART TX Pin",  driver->port_config.tx);
            printf("%s   %d", "UART RX Pin",  driver->port_config.rx);
            printf("%s", "--------");
            xSemaphoreGive(lock);
            i++;
        }
        else {
            ESP_LOGE(TAG, "Could not get lock for driver");
        }
    }
    ESP_LOGI(TAG, "Finished literating drivers");
}

void print_system_info(void) 
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("ESP32 Chip Info:\n");
    printf("  Model: %s\n", chip_info.model == CHIP_ESP32 ? "ESP32" : "Other");
    printf("  Cores: %d\n", chip_info.cores);
    printf("  Features: WiFi%s%s\n", 
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
}

void show_memory_usage(void) 
{
    // Local macro for consistent logging tag - scoped only to this function    
    // Query DMA capable memory statistics
    // DMA memory is required for hardware DMA operations and is typically limited
    #ifdef __XTENSA__
    const size_t total_dma = heap_caps_get_total_size(MALLOC_CAP_DMA);
    const size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
    
    // Log DMA memory information
    printf(" %s DMA capable memory:", MEMORY_TAG);
    printf(" %s  Total: %d bytes", MEMORY_TAG, total_dma);
    printf(" %s  Free: %d bytes", MEMORY_TAG, free_dma);

    // Query internal SRAM memory statistics
    // Internal memory is fast SRAM, preferred for performance-critical operations
    const size_t total_internal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    const size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    
    // Log internal memory information
    printf(" %s Internal memory:", MEMORY_TAG);
    printf(" %s  Total: %d bytes", MEMORY_TAG, total_internal);
    printf(" %s  Free: %d bytes", MEMORY_TAG, free_internal);
    #else
    printf(" %s Memory statistics are not available on this platform.", MEMORY_TAG);
    #endif
}