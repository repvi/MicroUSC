#include "generic.h"

esp_err_t set_driver_default(usc_driver_t *driver) {
    driver->sync_signal = xSemaphoreCreateBinary(); 
    if (driver->sync_signal == NULL) {
        ESP_LOGE("[DRIVER SET]", "Failed to create mutex lock"); 
        return ESP_ERR_NO_MEM;
    } 
    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
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

void set_driver_inactive(usc_driver_t *driver) { // need to finish the code at the beginning
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