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
    driver_system.lock = xSemaphoreCreateBinary(); // initialize the mux (mandatory)
    if (driver_system.lock == NULL) {
        ESP_LOGE(TAG, "Could not initialize main driver lock");
        return ESP_FAIL;
    }
    xSemaphoreGive(driver_system.lock);
    INIT_LIST_HEAD(&driver_system.driver_list.list);

    return init_hidden_driver_lists();
}

void init_tiny_kernel(void) {
    ESP_ERROR_CHECK(init_memory_handlers());
    ESP_ERROR_CHECK(init_priority_storage());
    ESP_LOGI(TAG, "System drivers initialized successfully\n");
    //usc_print_driver_configurations(); // Print the driver configurations
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
}