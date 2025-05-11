#include "tiny_kernel.h"
#include "USC_driver.h"
#include "speed_test.h"
#include "sdkconfig.h"
#include "esp_private/startup_internal.h"
#include "esp_system.h"

#include "memory_pool.h"

#define CORE           (0)
#define SECONDARY      (1)

#ifdef cycle_overdrivers // redefined thr macro in this case
#undef cycle_overdrivers
#define cycle_overdrivers() define_iteration_with_semaphore(overdrivers, usc_driver_t, driver, OVERDRIVER_MAX) 

#endif
//DEFINE_USC_DRIVER_INIT(drivers);
//DEFINE_USC_OVERDRIVER_INIT(overdrivers);
// ESP_EARLY_LOGD
#define SETUP_USC_DRIVER_INIT(driver_name) \
    static esp_err_t init_usc_##driver_name##_status(void) { \
        esp_err_t status = init_usc_##driver_name(); \
        if (status != ESP_OK) { \
            ESP_LOGE(TAG, "Failed to initialize %s: %s", #driver_name, esp_err_to_name(status)); \
            return status; \
        } \
        status = init_usc_##driver_name##_task_manager(); \
        if (status != ESP_OK) { \
            ESP_LOGE(TAG, "Failed to initialize %s task manager: %s", #driver_name, esp_err_to_name(status)); \
            return status; \
        } \
        return ESP_OK; \
    }

#define SETUP_USC_OVERDRIVER_INIT(d) SETUP_USC_DRIVER_INIT(d) // Initialize the overdrivers 


#define REGISTER_DRIVER(d) DEFINE_USC_DRIVER_INIT(d); SETUP_USC_DRIVER_INIT(d);
#define REGISTER_OVERDRIVER(d) DEFINE_USC_OVERDRIVER_INIT(d); SETUP_USC_OVERDRIVER_INIT(d);

//SETUP_USC_DRIVER_INIT(drivers);
//SETUP_USC_DRIVER_INIT(overdrivers); // Initialize the overdrivers
// name is only the definiition of the function, not the function itself
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

// cannot have different nammes
REGISTER_DRIVER(drivers) // initialize the drivers
REGISTER_OVERDRIVER(overdrivers) // Initialize the overdrivers

void init_tiny_kernel(void) {
    ESP_ERROR_CHECK(init_memory_handlers());
    ESP_ERROR_CHECK(init_usc_drivers_status());
    ESP_ERROR_CHECK(init_usc_overdrivers_status());
    ESP_LOGI(TAG, "System drivers initialized successfully\n");
    usc_print_driver_configurations(); // Print the driver configurations
    usc_print_overdriver_configurations();
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
}