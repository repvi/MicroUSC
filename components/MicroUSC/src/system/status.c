#include "MicroUSC/system/status.h"
#include "MicroUSC/internal/uscdef.h"
#include "MicroUSC/internal/driverList.h"
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
        SemaphoreHandle_t lock = driver->sync_signal;
        if (xSemaphoreTake(lock, SEMAPHORE_WAIT_TIME) == pdTRUE) {
            if (driver->status == DRIVER_UNINITALIALIZED) {
                ESP_LOGI(TAG, "NOT INITIALIZED on index %d", i);
                ESP_LOGI("--------", "----------------------------");
            }
            else {
                ESP_LOGI("DRIVER", "      %s",  driver->driver_name);
                ESP_LOGI("Baud Rate", "   %lu", driver->uart_config.baud_rate);
                ESP_LOGI("Status", "      %s",  status_str(config->status));
                ESP_LOGI("UART Port", "   %d",  driver->port_config.port);
                ESP_LOGI("UART TX Pin", " %d",  driver->port_config.tx);
                ESP_LOGI("UART RX Pin", " %d",  driver->port_config.rx);
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