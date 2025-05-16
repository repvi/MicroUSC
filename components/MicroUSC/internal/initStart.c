#include "initStart.h"
#include "esp_system.h"

#define TAG             "[DRIVER INIT]"

memory_block_handle_t mem_block_driver_nodes = NULL;
memory_block_handle_t mem_block_overdriver_nodes = NULL;

struct usc_driverList driver_list = {0};
struct usc_driverList overdriver_list = {0};

// used in the kernel
struct usc_driverNode {
    struct usc_driver_t driver_storage;
    UBaseType_t priority; // the priority of the task of the driver
    struct usc_driverNode *next;
};

#define SIZEOF_DRIVERNODE   ( sizeof( usc_driverNode ) )

bool getTask_status(const struct usc_task_manager_t *task) {
    return task->active;
}

void setTask_status(struct usc_task_manager_t *task, bool active) {
    task->active = active;
}

void setTaskHandlersNULL(struct usc_task_manager_t *task) {
    task->action_handle = NULL;  /* Reset the task handle */ 
    task->task_handle = NULL;  /* Reset the action task handle */ 
}

void setTaskDefault(struct usc_task_manager_t *task) {
    setTask_status(task, false);
    setTaskHandlersNULL(task);
}

struct usc_driver_t *getDriverListDriverAtTail(void) {
    return (driver_list.tail != NULL) ? &driver_list.tail->driver_storage : NULL;
}

struct usc_driver_t *getOverdriverListOverdriverAtTail(void) {
    return (driver_list.tail != NULL) ? &driver_list.tail->driver_storage : NULL;
}

UBaseType_t getNodePriority(usc_driverNode *node) {
    return node->priority;
}

struct usc_driverNode *driverListNext(struct usc_driverNode *node) {
    return node->next;
}

struct usc_driver_t *getDriverfromNode(struct usc_driverNode *node) {
    return &node->driver_storage;
}

esp_err_t init_driver_list(void) {
    mem_block_overdriver_nodes = (memory_block_handle_t)memory_handler_malloc(SIZEOF_DRIVERNODE, DRIVER_MAX);
    if (mem_block_driver_nodes == NULL) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t init_overdriver_list(void) {
    mem_block_overdriver_nodes = (memory_block_handle_t)memory_handler_malloc(SIZEOF_DRIVERNODE, OVERDRIVER_MAX);
    if (mem_block_overdriver_nodes == NULL) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t init_hidden_driver_lists(void) {
    esp_err_t status = init_driver_list();
    if (status != ESP_OK) {
        // add ESP_LOGW here maybe
        return status;
    }
    return init_overdriver_list();
}

// list param should never be NULL
static esp_err_t addDriverNodeM( struct usc_driverList *list,
                                 struct usc_driverNode * new_node,
                                 const struct usc_driver_t *driver_type) {
    xSemaphoreTake(list->lock, portMAX_DELAY);
    
    if (list->size < list->max) { // just ot make sure,,not needed
        if (list->head != NULL) { // could use a local pointer
            list->tail->next = new_node;
            list->tail = list->tail->next;
        }
        else { // could possibly use a local pointer
            list->head = new_node;
            list->tail = list->head;
        }
        xSemaphoreGive(list->lock);
        return ESP_OK;
    } // could be optimized here
    xSemaphoreGive(list->lock);
    return ESP_ERR_NOT_SUPPORTED; // Doesn't have space for it
}


esp_err_t addDriverNode(const struct usc_driver_t *driver) {
    struct usc_driverNode *new_node = (struct usc_driverNode *)memory_pool_alloc(mem_block_driver_nodes);
    if (new_node == NULL) {
        return ESP_ERR_INVALID_SIZE; // ran out of space for allocating
    }

    new_node->driver_storage.sync_signal = xSemaphoreCreateBinary();
    xSemaphoreGive(new_node->driver_storage.sync_signal); // just for safety

    return addDriverNodeM(&driver_list, new_node, driver);
}

esp_err_t addOverdriverNode(const struct usc_driver_t *overdriver) {
    struct usc_driverNode *new_node = (struct usc_driverNode *)memory_pool_alloc(mem_block_overdriver_nodes);
    if (new_node == NULL) {
        return ESP_ERR_INVALID_SIZE; // ran out of space for allocating
    }
    
    new_node->driver_storage.sync_signal = xSemaphoreCreateBinary();
    xSemaphoreGive(new_node->driver_storage.sync_signal); // just for safety

    return addDriverNodeM(&overdriver_list, new_node, overdriver);
}

esp_err_t set_driver_default(struct usc_driver_t *driver) {
    driver->sync_signal = xSemaphoreCreateBinary(); 
    if (driver->sync_signal == NULL) {
        ESP_LOGE("[DRIVER SET]", "Failed to create mutex lock"); 
        return ESP_ERR_NO_MEM;
    } 
    driver->driver_setting = (struct usc_config_t){}; // defualt to 0's
    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
    return ESP_OK;
}

esp_err_t set_driver_default_task(struct usc_driver_t *driver) {
    setTaskDefault(&driver->driver_tasks);
    return ESP_OK; /* Task is already inactive */
}

esp_err_t set_driver_inactive(struct usc_driver_t *driver) {
    struct usc_task_manager_t *task = &driver->driver_tasks; 
    struct usc_config_t *driver_setting = &driver->driver_setting; 
    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
    setTask_status(task, false); /* Reset the active flag */ 
    vTaskDelay(pdMS_TO_TICKS(DELAY_MILISECOND_50));  /* Delay for 1 second */ 
    while (!xSemaphoreTake(driver->sync_signal, SEMAPHORE_DELAY)); 
    setTaskHandlersNULL(task); 
    driver_setting->status = NOT_CONNECTED; /* Reset the driver status */ 
    xSemaphoreGive(driver->sync_signal); /* Unlock the queue */ 
    return ESP_OK;
}