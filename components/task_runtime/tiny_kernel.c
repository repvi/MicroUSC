#include "tiny_kernel.h"
#include "USC_driver.h"
#include "speed_test.h"
#include "sdkconfig.h"
#include "esp_private/startup_internal.h"
#include "esp_system.h"

#define CORE           (0)
#define SECONDARY      (1)

DEFINE_USC_DRIVER_INIT(drivers);
DEFINE_USC_OVERDRIVER_INIT(overdrivers);
#define SETUP_USC_DRIVER_INIT(driver_name)  \
    static esp_err_t init_usc_##driver_name##_status(void) { printf("Starting driver initialization\n"); \
        esp_err_t status = init_usc_##driver_name(); printf("Worked: 1"); \
        if (status != ESP_OK) { \
            ESP_EARLY_LOGD(TAG, "Failed to initialize %s: %s", #driver_name, esp_err_to_name(status)); \
            return status; \
        }  printf("Setting....");\
        status = init_usc_##driver_name##_task_manager(); printf("Didn't crash here");\
        if (status != ESP_OK) { \
            ESP_EARLY_LOGD(TAG, "Failed to initialize %s task manager: %s", #driver_name, esp_err_to_name(status)); \
            return status; \
        } \
        return ESP_OK; \
    }

SETUP_USC_DRIVER_INIT(drivers);
SETUP_USC_DRIVER_INIT(overdrivers); // Initialize the overdrivers
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
void init_tiny_kernel(void) {
    printf("Starting initialization\n");
    ESP_ERROR_CHECK(init_usc_drivers_status());
    ESP_ERROR_CHECK(init_usc_overdrivers_status());
    printf("System drivers initialized successfully\n");
}