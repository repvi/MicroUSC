#include "MicroUSC/system/status.h"
#include "MicroUSC/internal/uscdef.h"
#include "MicroUSC/internal/initStart.h"
#include "MicroUSC/synced_driver/USCdriver.h"
#include "esp_system.h"

#define TAG "[STATUS]"

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
        struct usc_driver_base_t *driver_base = &driver->driver_storage;
        SemaphoreHandle_t lock = driver->sync_signal;
        if (xSemaphoreTake(lock, SEMAPHORE_WAIT_TIME) == pdTRUE) {
            if (driver_base->driver_tasks.active == false) {
                ESP_LOGI(TAG, "NOT INITIALIZED on index %d", i);
                ESP_LOGI("--------", "----------------------------");
            }
            else {
                const struct usc_config_t *config = &driver_base->driver_setting;
                ESP_LOGI("DRIVER", "      %s", config->driver_name);
                ESP_LOGI("Baud Rate", "   %lu",config->baud_rate);
                ESP_LOGI("Status", "      %s", status_str(config->status));
                ESP_LOGI("Has Access", "  %d", config->has_access);
                ESP_LOGI("UART Port", "   %d", config->uart_config.port);
                ESP_LOGI("UART TX Pin", " %d", config->uart_config.tx);
                ESP_LOGI("UART RX Pin", " %d", config->uart_config.rx);
                ESP_LOGI("--------", "----------------------------");
            }
            xSemaphoreGive(lock);
            i++;
        }
        else {
            ESP_LOGE(TAG, "Could not get lock for driver");
        }
    }
    ESP_LOGI(TAG, "Finished literating drivers");
}