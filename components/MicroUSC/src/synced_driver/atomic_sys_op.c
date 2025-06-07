#include "MicroUSC/synced_driver/atomic_sys_op.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"
#include <stdbool.h>
#include "esp_log.h"

struct DataStorageQueue {
    uint32_t *serial_data;
    size_t size;
    size_t head;
    size_t tail;
};

#define DATAQUEUE_SIZE ( sizeof( struct DataStorageQueue ) )

typedef struct DataStorageQueue *SerialDataQueueHandler;

__always_inline size_t getDataStorageQueueSize(void)
{
    return sizeof(struct DataStorageQueue);
}

SerialDataQueueHandler createDataStorageQueue(const size_t serial_data_size) 
{
    const size_t alloc_size = serial_data_size * sizeof(uint32_t);
    SerialDataQueueHandler var = (SerialDataQueueHandler)heap_caps_malloc(DATAQUEUE_SIZE + alloc_size, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);

    if (var != NULL) {
        var->serial_data = (uint32_t *)((uint8_t *)var + DATAQUEUE_SIZE);
        if (var->serial_data == NULL) {
            heap_caps_free(var);
            return NULL;
        }

        var->head = 0;
        var->tail = 0;
        var->size = serial_data_size;
    }
    return var;
}

SerialDataQueueHandler createDataStorageQueueStatic(void *buffer, const size_t serial_data_size) {
    /* Cast the buffer to the queue handler type */
    SerialDataQueueHandler var = (SerialDataQueueHandler)buffer;

    /* Calculate the serial_data array position after the struct */
    var->serial_data = (uint32_t*)((uint8_t*)buffer + sizeof(struct DataStorageQueue));

    /* Initialize the data array to zero (fixed memset syntax) */
    memset(var->serial_data, 0, serial_data_size * sizeof(uint32_t));

    // Initialize queue metadata
    var->head = 0;
    var->tail = 0;
    var->size = serial_data_size;

    return var;
}

static __always_inline size_t moveNext(const size_t current, const size_t size) 
{
    return  ( -( ( current + 1 ) < size ) ) & ( current + 1 );
}

void dataStorageQueue_add(SerialDataQueueHandler queue, const uint32_t data)
{
    const size_t tail = queue->tail; // get the current index

    if (queue->serial_data[tail] == 0) {
        queue->serial_data[tail] = data; // store the data in the atomic variable
        queue->tail = moveNext(tail, queue->size); // increment the current index
    }
}

uint32_t dataStorageQueue_top(SerialDataQueueHandler queue)
{
    const size_t head = queue->head; // get the current index
    if (head >= queue->size) {
        ESP_LOGE("ATOMIC", "Bigger index: %u", head);
        return 0;
    }
    const uint32_t data = queue->serial_data[head]; // get the data from the queue
    if (data != 0) {
        queue->serial_data[head] = 0; //  // clear the data in the queue
        queue->head = moveNext(head, queue->size);
    }

    return data; // return the data
}

void dataStorageQueue_clean(SerialDataQueueHandler queue) 
{
    memset(queue->serial_data, 0, queue->size * sizeof(uint32_t));
    queue->head = 0;
    queue->tail = 0;
}


void destroyDataStorageQueue(SerialDataQueueHandler queue) 
{
    heap_caps_free(queue->serial_data);
    heap_caps_free(queue);
}