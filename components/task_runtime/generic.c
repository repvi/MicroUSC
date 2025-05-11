#include "generic.h"

memory_block_handle_t drivers_alloc = NULL; // do not rename
memory_block_handle_t overdrivers_alloc = NULL; // do not rename

esp_err_t malloc_drivers_handler(void) {
    drivers_alloc =  memory_handler_malloc(USC_TASK_MANAGER_SIZE, DRIVER_MAX);
    if (drivers_alloc == NULL) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t malloc_overdrivers_handler(void) {
    overdrivers_alloc = memory_handler_malloc(USC_TASK_MANAGER_SIZE, OVERDRIVER_MAX);
    if (overdrivers_alloc == NULL) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t init_memory_handlers(void) {
    if (malloc_drivers_handler() != ESP_OK || malloc_overdrivers_handler() != ESP_OK) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t set_driver_default(usc_driver_t *driver) {
    driver->sync_signal = xSemaphoreCreateBinary(); 
    if (driver->sync_signal == NULL) {
        ESP_LOGE("[DRIVER SET]", "Failed to create mutex lock"); 
        return ESP_ERR_NO_MEM;
    } 
    driver->driver_setting = (usc_config_t){}; // defualt to 0's
    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
    return ESP_OK;
}

esp_err_t set_driver_default_task(usc_driver_t *driver, memory_block_handle_t mem_pool_handler) {
    usc_tasks_t task = driver->driver_tasks;
    task = memory_pool_alloc(mem_pool_handler);
    if (task == NULL) {
        return ESP_ERR_NO_MEM;
    }
    setTaskDefault(task);
    return ESP_OK; /* Task is already inactive */
}

esp_err_t set_driver_inactive(usc_driver_t *driver) { 
    usc_tasks_t task = driver->driver_tasks; 
    usc_config_t *driver_setting = &driver->driver_setting; 
    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
    setTask_status(task, false); /* Reset the active flag */ 
    vTaskDelay(pdMS_TO_TICKS(DELAY_MILISECOND_50));  /* Delay for 1 second */ 
    while (!xSemaphoreTake(driver->sync_signal, SEMAPHORE_DELAY)); 
    setTaskHandlersNULL(task); 
    driver_setting->status = NOT_CONNECTED; /* Reset the driver status */ 
    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
    return ESP_OK;
}