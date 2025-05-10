#ifndef __TINY_KERNEL_GENERIC_H
#define __TINY_KERNEL_GENERIC_H

#include "USC_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "[DRIVER INIT]"

// Both will have the same name, but have differeent values and purposes
#define cycle_drivers_init_mode() define_iteration(drivers, usc_driver_t, driver, DRIVER_MAX) // used for the driver loop
#define cycle_overdrivers_init_mode() define_iteration(overdrivers, usc_driver_t, driver, OVERDRIVER_MAX) // used for the driver loop

esp_err_t set_driver_default(usc_driver_t *driver);

esp_err_t set_driver_default_task(usc_driver_t *driver);

esp_err_t set_driver_inactive(usc_driver_t *driver);

#define DEFINE_USC_DRIVER_INIT(driver_type) \
    static esp_err_t init_usc_##driver_type(void) { \
        esp_err_t ret; \
        size_t index = 0; \
        cycle_##driver_type##_init_mode() { \
            ret = set_driver_default(driver); \
            if (ret != ESP_OK) { \
                ESP_LOGE(TAG, "Could not initialize index %d variables", index);\
                return ret; \
            } \
            index++; \
        } \
        return ESP_OK; \
    } \
    \
    static esp_err_t init_usc_##driver_type##_task_manager(void) { \
        esp_err_t ret; \
        size_t index = 0; \
        cycle_##driver_type() { \
            if (hasSemaphore == pdTRUE) { \
                ret = set_driver_default_task(driver); \
                if (ret != ESP_OK) { \
                    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ \
                    ESP_LOGE(TAG, "Could not initialize index %d task manager", index);\
                    return ret; \
                } \
                index++; \
            } \
        } \
        return ESP_OK; \
    } \
    /* Make sure to initialize the drivers again when used */ \
    static esp_err_t usc_##driver_type##_deinit_all(void) { \
        esp_err_t ret; \
        size_t index = 0; \
        cycle_##driver_type() { \
            if (hasSemaphore == pdTRUE) { \
                ret = set_driver_inactive(driver); \
                if (ret != ESP_OK) { \
                    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ \
                    ESP_LOGE(TAG, "Could not deinitialize index %d driver", index);\
                    return ret; \
                } \
                index++; \
            } \
        } \
        return ESP_OK; \
    }

// driver_setting->has_access = false; /* Reset the access flag */ 

#define DEFINE_USC_OVERDRIVER_INIT(driver_type) DEFINE_USC_DRIVER_INIT(driver_type)

#ifdef __cplusplus
}
#endif

#endif