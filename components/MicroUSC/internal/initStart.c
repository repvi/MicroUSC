#include "initStart.h"
#include "esp_system.h"

#define TAG "[DRIVER INIT]"

struct usc_driversHandler driver_system = {0};

memory_block_handle_t mem_block_driver_nodes = NULL;

// make sure to take the lock and have access and release it manually
void addSingleDriver(const struct usc_driver_t *driver, const UBaseType_t priority)
{
    struct usc_driverList *new = memory_pool_alloc(mem_block_driver_nodes);
    if (new == NULL) {
        ESP_LOGE(TAG, "Could not allocate memory from the driver list");
        return;
    }
    new->driver_storage = *driver; // make sure the lock is already inialized
    new->priority = priority;
    INIT_LIST_HEAD(&new->list);
    list_add_tail(&new->list, &driver_system.driver_list.list);
}

// make sure to take the lock and have access and release it manually
void removeSingleDriver(struct usc_driverList *item) 
{
    list_del(&item->list);
    item = NULL;
}

void freeDriverList(void) 
{
    struct usc_driverList *current, *tmp;
    list_for_each_entry_safe(current, tmp, &driver_system.driver_list.list, list) {
        struct list_head *list = &current->list;
        list_del(list);
        heap_caps_free(list);
    }
}

bool getTask_status(const struct usc_task_manager_t *task)
{
    return task->active;
}

void setTask_status(struct usc_task_manager_t *task, bool active)
{
    task->active = active;
}

void setTaskHandlersNULL(struct usc_task_manager_t *task)
{
    task->action_handle = NULL; /* Reset the task handle */
    task->task_handle = NULL;   /* Reset the action task handle */
}

void setTaskDefault(struct usc_task_manager_t *task)
{
    setTask_status(task, false);
    setTaskHandlersNULL(task);
}

esp_err_t init_driver_list_memory_pool(void)
{
    mem_block_driver_nodes = (memory_block_handle_t)memory_handler_malloc(sizeof(struct usc_driverList), DRIVER_MAX);
    if (mem_block_driver_nodes == NULL) {
        ESP_LOGE(TAG, "Could not initialize driver list memory pool");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t init_hidden_driver_lists(void)
{
    return init_driver_list_memory_pool();
}

esp_err_t set_driver_default(struct usc_driver_t *driver)
{
    SemaphoreHandle_t sync = driver->sync_signal;
    xSemaphoreTake(sync, portMAX_DELAY);
    // implement here
    xSemaphoreGive(sync); /* Unlock the queue */
    return ESP_OK;
}

esp_err_t set_driver_default_task(struct usc_driver_t *driver)
{
    setTaskDefault(&driver->driver_tasks);
    return ESP_OK; /* Task is already inactive */
}

void set_driver_inactive(struct usc_driver_t *driver)
{
    struct usc_task_manager_t *task = &driver->driver_tasks;
    struct usc_config_t *driver_setting = &driver->driver_setting;
    xSemaphoreGive(driver->sync_signal);            /* Unlock the queue */
    setTask_status(task, false);                    /* Reset the active flag */
    vTaskDelay(pdMS_TO_TICKS(DELAY_MILISECOND_50)); /* Delay for 1 second */
    while (!xSemaphoreTake(driver->sync_signal, SEMAPHORE_DELAY));
    setTaskHandlersNULL(task);
    driver_setting->status = NOT_CONNECTED; /* Reset the driver status */
    xSemaphoreGive(driver->sync_signal);    /* Unlock the queue */
}