#include "MicroUSC-kernel.h"

#include "USCdriver.h"
#include "speed_test.h"
#include "sdkconfig.h"
#include "esp_private/startup_internal.h"
#include "esp_system.h"

#include "memory_pool.h"

#define TAG            "[MICROUSC KERNEL]"

#define CORE           (0)
#define SECONDARY      (1)

/*
ESP_SYSTEM_INIT_FN(tiny_kernel, SECONDARY, BIT(0), 120) {
    ESP_EARLY_LOGD(TAG, "Starting initizalization\n");
    if (init_usc_drivers_status() != ESP_OK || init_usc_overdrivers_status() != ESP_OK) {
        return ESP_FAIL; // Initialization failed
    }
    ESP_EARLY_LOGD(TAG, "System drivers initialized successfully\n");
    return ESP_OK;
}
*/



esp_err_t init_memory_handlers(void) {
    driver_list.lock = xSemaphoreCreateBinary(); // initialize the mux (mandatory)
    if (driver_list.lock == NULL) {
        return ESP_ERR_NO_MEM;
    }
    xSemaphoreGive(driver_list.lock);

    if (init_driver_list() != ESP_OK || init_overdriver_list() != ESP_OK) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void init_tiny_kernel(void) {
    ESP_ERROR_CHECK(init_memory_handlers());
    ESP_LOGI(TAG, "System drivers initialized successfully\n");
    usc_print_driver_configurations(); // Print the driver configurations
    usc_print_overdriver_configurations();
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
}