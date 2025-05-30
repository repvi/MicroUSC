#include "MicroUSC/internal/driverList.h"
#include "MicroUSC/system/MicroUSC-internal.h"
#include "MicroUSC/synced_driver/USCdriver.h"
#include "debugging/speed_test.h"
#include "esp_system.h"

#define TAG "[DRIVER INIT]"

#define DRIVERLIST_SIZE           sizeof( struct usc_driverList )
#define STATIC_SEMAPHORE_SIZE     sizeof( StaticSemaphore_t )

#define PROCESSOR  "processor"
#define READER "reader"
struct usc_driversHandler driver_system = {0};

memory_block_handle_t mem_block_driver_nodes = NULL;

size_t data_size;
size_t buff_size;

__always_inline void ptrOffset(void *ptr, size_t offset) {
    ptr = ( uint8_t * ) ( ( ( uintptr_t )ptr + offset ) );
}

//defined in "MicroUSC/synced_driver/USCdriver.h"
void usc_driver_read_task(void *pvParameters);

static void create_usc_driver_reader( struct usc_driver_t *driver,
                                      const UBaseType_t i
) { // might neeed adjustment
    const UBaseType_t DRIVER_TASK_Priority_START = TASK_PRIORITY_START + i;
    char task_name[30];
    strncpy(task_name, driver->driver_name, sizeof(task_name));
    strncat(task_name, READER, sizeof(READER));

    driver->uart_reader.task = xTaskCreateStaticPinnedToCore(
        usc_driver_read_task,                                // Task function
        task_name,                                 // Task name
        TASK_STACK_SIZE,                                     // Stack size
        (void *)driver,                                      // Task parameters
        DRIVER_TASK_Priority_START,                          // Increment priority for next task
        driver->uart_reader.stack,
        &driver->uart_reader.task_buffer,
        TASK_CORE_READER                                     // Core to pin the task
    );
}

// todo -> needs to be changed to have remove the task driver (reader) action of the uart
static void create_usc_driver_processor( struct usc_driver_t *driver,
                                         const usc_data_process_t driver_process,
                                         const UBaseType_t i
) {
    const UBaseType_t OFFSET = TASK_PRIORITY_START + i;
    char task_name[30];
    strncpy(task_name, driver->driver_name, sizeof(task_name));
    strncat(task_name, PROCESSOR, sizeof(PROCESSOR));
    driver->uart_processor.task = xTaskCreateStaticPinnedToCore(
        driver_process,                                        // Task function
        task_name,                                             // Task name
        driver->uart_processor.stack_size,                                       // Stack size
        (void *)i,                                             // Task parameters
        (OFFSET),                                              // Based on index
        driver->uart_processor.stack,    // Task handle
        driver->uart_processor.task_buffer,
        TASK_CORE_ACTION                                       // Core to pin the task
    );
}

static void setUpMemDriver(struct usc_driverList *driverList) {
    struct usc_driver_t *driver = &driverList->driver;
    uint8_t *ptr = (uint8_t *)driverList + DRIVERLIST_SIZE;

    driver->sync_signal = xSemaphoreCreateBinaryStatic( ( StaticSemaphore_t * ) ( ptr ) ); // probably add if it is NULL or not
    #ifdef MICROUSC_DEBUG
    if (driver->sync_signal == NULL) {
        ESP_LOGE(TAG, "Could not inialize");
        set_microusc_system_code(USC_SYSTEM_ERROR);
        return;
    }
    #endif
    xSemaphoreGive(driver->sync_signal); // create the section
    ptrOffset(ptr, STATIC_SEMAPHORE_SIZE); // move to the free memory after the semaphore

    driver->buffer.memory = ptr;
    ptrOffset(ptr, driver->buffer.size); // make the array the size of the buffer

    createDataStorageQueueStatic(driver->data, ptr, data_size);
}

// make sure to take the lock and have access and release it manually
void addSingleDriver( const char *const driver_name,
                      const uart_config_t uart_config,
                      const uart_port_config_t port_config,
                      const stack_size_t stack_size,
                      const UBaseType_t priority)
{
    struct usc_driverList *new = (struct usc_driverList *)memory_pool_alloc(mem_block_driver_nodes);
    struct usc_driver_t *driver = &new->driver; // point to the first byte of the allocated memory

    driver->uart_processor.stack_size = stack_size;
    driver->uart_config = uart_config;
    if (strncmp(driver->driver_name, "", DRIVER_NAME_SIZE - 1) != 0) {
        strncpy(driver->driver_name, driver_name, sizeof(driver_name_t) - 1); // maybe needs the -1
    }
    else {
        static int no_name = 1;
        snprintf(driver->driver_name, DRIVER_NAME_SIZE - 1, "Unknown Driver %d", no_name);
        no_name++;
    }
    driver->driver_name[DRIVER_NAME_SIZE - 1] = '\0';
    driver->port_config = port_config;
    driver->buffer.size = buff_size;
    driver->status = NOT_CONNECTED;
    driver->priority = priority;
    driver->has_access = false;

    setUpMemDriver(driver);

    ESP_LOGI(TAG, "Completeted initializing driver");

    INIT_LIST_HEAD(&new->list);
    list_add_tail(&new->list, &driver_system.driver_list.list);
    driver_system.size++;
}

// make sure to take the lock and have access and release it manually
void removeSingleDriver(struct usc_driverList *item) 
{
    if (driver_system.size != 0) {
        list_del(&item->list);
        item = NULL;
        driver_system.size--;
    }
}

void freeDriverList(void) 
{
    struct usc_driverList *current, *tmp;
    list_for_each_entry_safe(current, tmp, &driver_system.driver_list.list, list) {
        struct list_head *list = &current->list;
        list_del(list);
        heap_caps_free(list);
    }
    driver_system.size = 0;
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

esp_err_t init_driver_list_memory_pool(const size_t buffer_size, const size_t d_size)
{
    const size_t total = sizeof(struct usc_driverList) + sizeof(StaticSemaphore_t);
    const size_t total = sizeof(struct usc_driverList) + (sizeof(StaticSemaphore_t)) + TASK_STACK_SIZE + buffer_size + d_size + sizeof(DataStorageQueueStatic);
    mem_block_driver_nodes = (memory_block_handle_t)memory_handler_malloc(total, DRIVER_MAX);
    if (mem_block_driver_nodes == NULL) {
        ESP_LOGE(TAG, "Could not initialize driver list memory pool");
        return ESP_ERR_NO_MEM;
    }
    buff_size = buffer_size;
    data_size = d_size;
    return ESP_OK;
}

__always_inline esp_err_t init_hidden_driver_lists(const size_t buffer_size, const size_t data_size)
{
    return init_driver_list_memory_pool(buffer_size, data_size);
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
    setTaskDefault(&driver->driver_storage.driver_tasks);
    return ESP_OK; /* Task is already inactive */
}

void set_driver_inactive(struct usc_driver_t *driver)
{
    struct usc_config_t *driver_setting = &driver->driver_storage.driver_setting;
    struct usc_task_manager_t *task = &driver->driver_storage.driver_tasks;
    xSemaphoreGive(driver->sync_signal);            /* Unlock the queue */
    setTask_status(task, false);                    /* Reset the active flag */
    vTaskDelay(pdMS_TO_TICKS(DELAY_MILISECOND_50)); /* Delay for 1 second */
    while (!xSemaphoreTake(driver->sync_signal, SEMAPHORE_DELAY));
    setTaskHandlersNULL(task);
    driver_setting->status = NOT_CONNECTED; /* Reset the driver status */
    xSemaphoreGive(driver->sync_signal);    /* Unlock the queue */
}