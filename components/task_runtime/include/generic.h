#ifndef __TINY_KERNEL_GENERIC_H
#define __TINY_KERNEL_GENERIC_H

#include "USC_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef cycle_overdrivers
#undef cycle_overdrivers
#define cycle_overdrivers() define_iteration_with_semaphore(overdrivers, usc_driver_t, driver, OVERDRIVER_MAX) 

#endif

esp_err_t set_driver_default(usc_driver_t *driver) {
    driver->sync_signal = xSemaphoreCreateBinary(); 
    if (driver->sync_signal == NULL) {
        ESP_LOGE("[DRIVER SET]", "Failed to create mutex lock"); 
        return ESP_ERR_NO_MEM; 
    } 
    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
    driver->driver_tasks.active = false; 

    return ESP_OK;
}

void set_driver_default_task(usc_driver_t *driver) {    
    usc_task_manager_t *task = &driver->driver_tasks; 
    if (task->active) {
        task->task_handle = NULL; /* Reset the task handle */ 
        task->action_handle = NULL; /* Reset the action task handle */ 
        task->active = false; /* Reset the active flag */ 
    }
}

void set_driver_inactive(usc_driver_t *driver) {
    usc_task_manager_t *task = &driver->driver_tasks; 
    usc_config_t *driver_setting = &driver->driver_setting; 
    if (task->task_handle != NULL) { 
        task->active = false; /* Reset the active flag */ 
        xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
        vTaskDelay(pdMS_TO_TICKS(DELAY_MILISECOND_50));  /* Delay for 1 second */ 
        while (!xSemaphoreTake(driver->sync_signal, SEMAPHORE_DELAY)); 
        task->task_handle = NULL; /* Reset the task handle */ 
        task->action_handle = NULL; /* Reset the action task handle */ 
    } 
    driver_setting->status = NOT_CONNECTED; /* Reset the driver status */ 
    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
}

#define DEFINE_USC_DRIVER_INIT(driver_type) \
    esp_err_t init_usc_##driver_type(void) { \
        esp_err_t ret; \
        cycle_##driver_type() { \
            ret = set_driver_default(driver); \
            if (ret != ESP_OK) { \
                return ret; \
            } \
        } \
        return ESP_OK; \
    } \
    \
    esp_err_t init_usc_##driver_type##_task_manager(void) { \
        cycle_##driver_type() { \
            set_driver_default_task(driver); \
        } \
        return ESP_OK; \
    } \
    /* Make sure to initialize the drivers again when used */ \
    esp_err_t usc_##driver_type##_deinit_all(void) { \
        cycle_##driver_type() { \
            if (hasSemaphore == pdTRUE) { \
                set_driver_inactive(driver); \
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