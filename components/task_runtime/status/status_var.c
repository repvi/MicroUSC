#include "status_var.h"
#include "esp_system.h"
#include "USC_driver.h"

void usc_print_driver_configurations(void) {
    int i = 0;
    cycle_drivers() {
        if (hasSemaphore == pdTRUE) {
            if (getTask_status(driver->driver_tasks) == false) {
                ESP_LOGI("DRIVER", "NOT INITIALIZED on index %d", i);
                ESP_LOGI("      ", "----------------------------");
            }
            else {
                const usc_config_t *config = &driver->driver_setting;
                ESP_LOGI("DRIVER", " %s",      config->driver_name);
                ESP_LOGI("Baud Rate", " %lu",  config->baud_rate);
                ESP_LOGI("Status", " %d",      config->status);
                ESP_LOGI("Has Access", " %d",  config->has_access);
                ESP_LOGI("UART Port", " %d",   config->uart_config.port);
                ESP_LOGI("UART TX Pin", " %d", config->uart_config.tx);
                ESP_LOGI("UART RX Pin", " %d", config->uart_config.rx);
                ESP_LOGI("      ", "----------------------------");
            }
            i++;
        }
    }
}

void usc_print_overdriver_configurations(void) {
    int i = 0;
    cycle_overdrivers() {
        if (hasSemaphore == pdTRUE) {
            if (getTask_status(overdriver->driver_tasks) == false) {
                ESP_LOGI("OVERDRIVER", " NOT INITIALIZED on index %d", i);
                ESP_LOGI("          ", "----------------------------");
            }
            else {
                const usc_config_t *config = &overdriver->driver_setting;
                ESP_LOGI("OVERDRIVER", " %s", config->driver_name);
                ESP_LOGI("Baud Rate", " %lu", config->baud_rate);
                ESP_LOGI("          ", "----------------------------");
            }
            i++;
        }
    }
}