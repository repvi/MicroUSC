#include "tiny_kernel.h"
#include "USC_driver.h"
#include "speed_test.h"

DEFINE_USC_DRIVER_INIT(drivers);
DEFINE_USC_OVERDRIVER_INIT(overdrivers);

void RUN_FIRST set_system_drivers(void) {
    ESP_ERROR_CHECK(init_usc_drivers()); // Initialize the drivers
    ESP_ERROR_CHECK(init_usc_drivers_task_manager());
    ESP_ERROR_CHECK(init_usc_overdrivers()); // Initialize the overdrivers
    ESP_ERROR_CHECK(init_usc_overdrivers_task_manager());
    ESP_ERROR_CHECK(usc_drivers_deinit_all()); // used to set the default drivers
    ESP_ERROR_CHECK(usc_overdrivers_deinit_all()); // used to set the default drivers
}