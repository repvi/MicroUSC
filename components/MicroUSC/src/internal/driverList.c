#include "MicroUSC/system/manager.h"
#include "MicroUSC/internal/system/bit_manip.h"
#include "MicroUSC/internal/driverList.h"
#include "MicroUSC/USCdriver.h"
#include "debugging/speed_test.h"
#include "esp_system.h"

#define TAG "[DRIVER INIT]"

#define DRIVERLIST_SIZE           sizeof( struct usc_driverList )
#define STATIC_SEMAPHORE_SIZE     sizeof( StaticSemaphore_t )

#define PROCESSOR  "processor"
#define READER     "reader"

struct usc_driversHandler driver_system = {0};

memory_block_handle_t mem_block_driver_nodes = NULL;
memory_block_handle_t mem_block_task_processor = NULL;

struct {
    size_t data_size;
    size_t buffer_size;
} stored_sizes;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define ALIGNOF(type) _Alignof(type)
#else
    #define ALIGNOF(type) __alignof__(type)
#endif

#define ALIGN_PTR(p, a) (void*)(((uintptr_t)(p) + ((a)-1)) & ~((a)-1))

/**
 * @brief Returns a pointer offset by a given number of bytes.
 *
 * @param ptr Base pointer.
 * @param offset Number of bytes to offset.
 * @return Pointer offset by 'offset' bytes.
 */
__always_inline void* ptrOffset(void *ptr, size_t offset) {
    /* Casts the pointer to uint8_t* for byte-wise arithmetic, then adds the offset. */
    return (void *)((uint8_t *)ptr + offset);
}

//defined in "MicroUSC/USCdriver.h"
UBaseType_t getCurrentEmptyDriverIndex(void);
UBaseType_t getCurrentEmptyDriverIndexAndOccupy(void);

void usc_driver_read_task(void *pvParameters);

/**
 * @brief Configures a task name by concatenating two strings.
 *
 * @param des Destination buffer for the task name.
 * @param str First string (e.g., driver name).
 * @param tmp Second string (e.g., role like "reader" or "processor").
 * @param len1 Length of the first string to copy.
 * @param len2 Length of the second string to concatenate.
 */
static void task_name_configure(char *restrict des, char *restrict str, char *restrict tmp, size_t len1, size_t len2) {
    /* Copies the first string and concatenates the second string to form the task name. */    
    strncpy(des, str, len1);
    strncat(des, tmp, len2);
}

/**
 * @brief Creates and starts the USC driver reader task.
 *
 * @param driver Pointer to the driver structure.
 * @param i Index for task priority calculation.
 */
static void create_usc_driver_reader(struct usc_driver_t *driver, const UBaseType_t i) {
    const UBaseType_t DRIVER_TASK_Priority_START = TASK_PRIORITY_START + i;
    char task_name[30];
    /* Configure the task name using driver name and role. */
    task_name_configure(task_name, driver->driver_name, READER, sizeof(task_name), sizeof(READER));
    /* Create the static pinned-to-core task for reading. */
    driver->uart_reader.task = xTaskCreateStaticPinnedToCore(
        usc_driver_read_task,             /* Task function */
        task_name,                        /* Task name */
        TASK_STACK_SIZE,                  /* Stack size */
        (void *)driver,                   /* Task parameters */
        DRIVER_TASK_Priority_START,       /* Task priority */
        driver->uart_reader.stack,        /* Stack buffer */
        &driver->uart_reader.task_buffer, /* Task buffer */
        TASK_CORE_READER                  /* Core to pin the task */
    );
}

/**
 * @brief Creates and starts the USC driver processor task.
 *
 * @param driver Pointer to the driver structure.
 * @param driver_process Task function for the processor.
 * @param i Index for task priority calculation.
 */
static void create_usc_driver_processor(struct usc_driver_t *driver, const usc_process_t driver_process, const UBaseType_t i) {
    const UBaseType_t OFFSET = TASK_PRIORITY_START + i;
    char task_name[30];
    /* Configure the task name using driver name and role. */
    task_name_configure(task_name, driver->driver_name, PROCESSOR, sizeof(task_name), sizeof(PROCESSOR));
    /* Create the static pinned-to-core task for processing. */
    driver->uart_processor.task = xTaskCreateStaticPinnedToCore(
        driver_process,                      /* Task function */
        task_name,                           /* Task name */
        driver->uart_processor.stack_size,   /* Stack size */
        (void *)driver,                      /* Task parameters */
        OFFSET,                              /* Task priority */
        driver->uart_processor.stack,        /* Stack buffer */
        &driver->uart_processor.task_buffer, /* Task buffer */
        TASK_CORE_ACTION                     /* Core to pin the task */
    );
}

/**
 * @brief Quickly gives a semaphore from an ISR and yields if necessary.
 *
 * @param signal Semaphore handle to give.
 */
static void IRAM_ATTR driver_quick_semaphore_give_fast(SemaphoreHandle_t signal) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    /* Give the semaphore from ISR context. */
    xSemaphoreGiveFromISR(signal, &xHigherPriorityTaskWoken);
    /* Yield to a higher priority task if required. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Triggers a driver's ISR by giving its sync semaphore.
 *
 * @param driver Pointer to the driver structure.
 */
__always_inline void driver_isr_trigger(struct usc_driver_t *driver) 
{
    /* Calls the fast semaphore give function for the driver's sync signal. */
    driver_quick_semaphore_give_fast(driver->sync_signal);
}

/**
 * @brief Sets up memory and synchronization for a driver.
 *
 * Initializes the driver's semaphore, buffer, and creates its tasks.
 *
 * @param driverList Pointer to the driver list node.
 * @param driver_processor Task function for the processor.
 * @param priority Priority index for the driver.
 */
static void setUpMemDriver( struct usc_driverList *driverList, 
                            const usc_process_t driver_processor,
                            const UBaseType_t priority
) {
    struct usc_driver_t *driver = &driverList->driver;
    uint8_t *ptr = (uint8_t *)driverList + DRIVERLIST_SIZE;

    /* Sets up the main semaphore used for the driver */
    driver->sync_signal = xSemaphoreCreateBinaryStatic((StaticSemaphore_t *)ptr); // xSemaphoreCreateBinaryStatic
    xSemaphoreGive(driver->sync_signal);
    ptr = ptrOffset(ptr, STATIC_SEMAPHORE_SIZE);

    /* Set up the pointer for the remaining allocated memory with a scope */
    driver->buffer.memory = ptr;
    memset(driver->buffer.memory, 0xFF, driver->buffer.size);
    ptr = ptrOffset(ptr, driver->buffer.size);

    /* Create the tasks that will run the USC drivers */
    create_usc_driver_reader(driver, priority);
    create_usc_driver_processor(driver, driver_processor, priority);
}

void addSingleDriver( const char *const driver_name,
                      const uart_config_t uart_config,
                      const uart_port_config_t port_config,
                      const usc_process_t driver_process,
                      const stack_size_t stack_size
) {
    /* Allocate a new driver list node from the memory pool. */
    struct usc_driverList *new = (struct usc_driverList *)memory_pool_alloc(mem_block_driver_nodes);
    struct usc_driver_t *driver = &new->driver; /* point to the first byte of the allocated memory */

    /* Allocate and initialize the data queue for the driver. */
    const size_t serial_data_storage_size = 256; 
    void* tmp_buffer = heap_caps_malloc(getDataStorageQueueSize() + ( serial_data_storage_size * sizeof(uint32_t) ), MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    driver->data = createDataStorageQueueStatic(tmp_buffer, serial_data_storage_size);

    driver->uart_reader.active = true;
    /* Allocate stack memory for the processor task, using static pool if available. */
    if (mem_block_task_processor == NULL) { // has not been statically initialized
        ESP_LOGI(TAG, "Allocating stack of size %u", stack_size);
        driver->uart_processor.stack = (StackType_t *)heap_caps_malloc(stack_size, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        if (driver->uart_processor.stack == NULL) {
            ESP_LOGE(TAG, "Failed to allocate for stack of the reader task");
            return;
        }
        driver->uart_processor.stack_size = stack_size;
    } else {
        driver->uart_processor.stack = (StackType_t *)memory_pool_alloc(mem_block_task_processor);
        driver->uart_processor.stack_size = mem_block_task_processor->block_size;
    }

    /* Store configuration and initialize driver fields. */
    driver->uart_config = uart_config;
    if (strncmp(driver->driver_name, "", DRIVER_NAME_SIZE - 1) != 0) {
        strncpy(driver->driver_name, driver_name, sizeof(driver_name_t) - 1);
    } else {
        static int no_name = 1;
        snprintf(driver->driver_name, DRIVER_NAME_SIZE - 1, "Unknown Driver %d", no_name);
        no_name++;
    }
    driver->driver_name[DRIVER_NAME_SIZE - 1] = '\0'; /* NULL terminator for the c string */
    driver->port_config = port_config; /* the port and the rx and tx pins */
    driver->buffer.size = stored_sizes.buffer_size; /* the buffer size of the driver (4 bytes) */
    driver->status = NOT_CONNECTED; /* by default the driver is seene as not connected */

    driver->priority = getCurrentEmptyDriverIndexAndOccupy(); /* retrieve the first empty bit */
    driver->has_access = false; /* by default all devices do not have access */

    /* sets up all the varaibles that use dynamic memory inside the driver */
    setUpMemDriver(new, driver_process, driver->priority);

    ESP_LOGI(TAG, "Completeted initializing driver");

    /* Initialize the double linked list */
    INIT_LIST_HEAD(&new->list);
    list_add_tail(&new->list, &driver_system.driver_list.list);
    driver_system.size++; /* Increment the current size count of the driver list */
}

void removeSingleDriver(struct usc_driverList *item) 
{
    if (driver_system.size != 0) {
        /* Remove the node from the list and decrement the size. */
        list_del(&item->list);
        item = NULL;
        driver_system.size--;
    }
}

void freeDriverList(void) 
{
    struct usc_driverList *current, *tmp;
    /* Iterate safely through the list and free each node. */
    list_for_each_entry_safe(current, tmp, &driver_system.driver_list.list, list) {
        struct list_head *list = &current->list;
        list_del(list);
        heap_caps_free(list);
    }
    driver_system.size = 0;
}

esp_err_t init_driver_list_memory_pool(const size_t buffer_size, const size_t data_size)
{
    size_t total = 0;

    /* Add size for the driver list struct. */
    total += sizeof(struct usc_driverList);

    /* Add alignment and size for the static semaphore. */
    total += ALIGNOF(StaticSemaphore_t) - 1;
    total += sizeof(StaticSemaphore_t);

    /* Add alignment and size for the buffer. */
    total += ALIGNOF(uint32_t) - 1;
    total += buffer_size;

    /* Optionally, add a small safety margin */
    total += 16;

    /* Allocate the memory pool for all driver nodes */
    mem_block_driver_nodes = (memory_block_handle_t)memory_handler_malloc(total, DRIVER_MAX);
    if (mem_block_driver_nodes == NULL) {
        ESP_LOGE(TAG, "Could not initialize driver list memory pool");
        return ESP_ERR_NO_MEM;
    }
    
    /* Store buffer and data sizes for later use. */
    stored_sizes.buffer_size = buffer_size;
    stored_sizes.data_size = data_size;
    return ESP_OK;
}

esp_err_t setUSCtaskSize(stack_size_t size) {
    mem_block_task_processor = (memory_block_handle_t)memory_handler_malloc(size, DRIVER_MAX);
    if (mem_block_task_processor == NULL) {
        ESP_LOGI(TAG, "Could not initialize static memory pool for stack");
        return ESP_ERR_NO_MEM;
    }
    const size_t total_mem = size * DRIVER_MAX;
    ESP_LOGI(TAG, "Created %d with stack size %d using %u total memory", DRIVER_MAX, size, total_mem);
    return ESP_OK;
}

__always_inline esp_err_t init_hidden_driver_lists(const size_t buffer_size, const size_t data_size)
{
    /* Calls the main driver list memory pool initializer. */
    return init_driver_list_memory_pool(buffer_size, data_size);
}