#ifndef __TINY_KERNEL_GENERIC_H
#define __TINY_KERNEL_GENERIC_H

#include "USC_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef cycle_overdrivers // redefined thr macro in this case
#undef cycle_overdrivers
#define cycle_overdrivers() define_iteration_with_semaphore(overdrivers, usc_driver_t, driver, OVERDRIVER_MAX) 

#endif

// Both will have the same name, but have differeent values and purposes
#define cycle_drivers_init_mode() define_iteration(drivers, usc_driver_t, driver, DRIVER_MAX) // used for the driver loop
#define cycle_overdrivers_init_mode() define_iteration(overdrivers, usc_driver_t, driver, OVERDRIVER_MAX) // used for the driver loop

esp_err_t set_driver_default(usc_driver_t *driver);

void set_driver_default_task(usc_driver_t *driver);

void set_driver_inactive(usc_driver_t *driver);

#define DEFINE_USC_DRIVER_INIT(driver_type) \
    esp_err_t init_usc_##driver_type(void) { \
        esp_err_t ret; \
        cycle_##driver_type##_init_mode() { printf("From init_##driver_type(void)\n"); \
            ret = set_driver_default(driver); printf("Set to default\n"); \
            if (ret != ESP_OK) { \
                return ret; \
            } printf("Next....\n"); \
        } \
        return ESP_OK; \
    } \
    \
    esp_err_t init_usc_##driver_type##_task_manager(void) { \
        cycle_##driver_type() { \
            set_driver_default_task(driver_type); \
        } \
        return ESP_OK; \
    } \
    /* Make sure to initialize the drivers again when used */ \
    esp_err_t usc_##driver_type##_deinit_all(void) { \
        cycle_##driver_type() { \
            if (hasSemaphore == pdTRUE) { \
                set_driver_inactive(driver_type); \
            } \
        } \
        return ESP_OK; \
    }

// driver_setting->has_access = false; /* Reset the access flag */ 

#define DEFINE_USC_OVERDRIVER_INIT(driver_type) DEFINE_USC_DRIVER_INIT(driver_type)

// already defined in the header file
#undef cycle_overdrivers
#define cycle_overdrivers() define_iteration_with_semaphore(overdrivers, usc_driver_t, overdriver, OVERDRIVER_MAX) 

#ifdef __cplusplus
}
#endif

#endif