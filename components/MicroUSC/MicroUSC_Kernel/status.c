#include "status.h"
#include "uscdef.h"
#include "initStart.h"
#include "esp_system.h"
#include "USCdriver.h"

#define TAG "[STATUS]"

void usc_print_driver_configurations(void) {
    int i = 0;
    struct usc_driverList *current, *tmp;
    list_for_each_entry_safe(current, tmp, &driver_system.driver_list.list, list) {
        struct usc_driver_t *driver = &current->driver;
        struct usc_driver_base_t *driver_base = &driver->driver_storage;
        SemaphoreHandle_t lock = driver->sync_signal;
        if (xSemaphoreTake(lock, SEMAPHORE_WAIT_TIME) == pdTRUE) {
            if (driver_base->driver_tasks.active == false) {
                ESP_LOGI("DRIVER", "NOT INITIALIZED on index %d", i);
                ESP_LOGI("      ", "----------------------------");
            }
            else {
                const struct usc_config_t *config = &driver_base->driver_setting;
                ESP_LOGI("DRIVER", " %s",      config->driver_name);
                ESP_LOGI("Baud Rate", " %lu",  config->baud_rate);
                ESP_LOGI("Status", " %d",      config->status);
                ESP_LOGI("Has Access", " %d",  config->has_access);
                ESP_LOGI("UART Port", " %d",   config->uart_config.port);
                ESP_LOGI("UART TX Pin", " %d", config->uart_config.tx);
                ESP_LOGI("UART RX Pin", " %d", config->uart_config.rx);
                ESP_LOGI("      ", "----------------------------");
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