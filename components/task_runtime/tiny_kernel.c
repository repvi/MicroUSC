#include "tiny_kernel.h"
#include "USC_driver.h"
#include "speed_test.h"

static esp_err_t init_usc_task_manager(void) {
    literate_bytes(driver_task_manager, usc_task_manager_t, DRIVER_MAX) {
        begin->task_handle = NULL;
        begin->action_handle = NULL;
        begin->active = false;
    }

    return ESP_OK;
}

void RUN_FIRST set_system_drivers(void) {
    ESP_ERROR_CHECK(init_usc_task_manager());
    ESP_ERROR_CHECK(usc_driver_deinit_all()); // used to set the default drivers
    ESP_ERROR_CHECK(usc_overdriver_deinit_all()); // used to set the default drivers
}