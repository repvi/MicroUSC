#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <atomic_sys_op.h>
#include <stdbool.h>

portMUX_TYPE queueLock = portMUX_INITIALIZER_UNLOCKED; // example of a mutex lock

void queue_add(Queue *queue, const uint32_t data) {
    portENTER_CRITICAL(&queueLock); // Lock the queue

    const size_t tail = queue->tail; // get the current index
    if (queue->serial_data[tail] != 0) {
        return; // queue is full
    }

    size_t next = queue->tail + 1; // get the next index
    if (next >= QUEUE_MAX_SIZE) {
        next = 0; // wrap around
    }

    queue->serial_data[next - 1] = data; // store the data in the atomic variable
    queue->tail = next; // increment the current index

    portEXIT_CRITICAL(&queueLock); // Unlock the queue
}

uint32_t queue_top(Queue *queue) {
    portENTER_CRITICAL(&queueLock); // Lock the queue
    size_t head = queue->head; // get the current index
    uint32_t *serial_data = &queue->serial_data[head]; // get the pointer to the serial data
    const uint32_t data = *serial_data; // get the data from the queue
    if (data == 0) {
        return 0; // queue is empty
    }

    *serial_data = 0; // clear the data in the queue
    head = (head < QUEUE_MAX_SIZE) & (head + 1); // reset the index if out of bounds

    queue->head = head; // clear the data in the queue
    
    portEXIT_CRITICAL(&queueLock); // Unlock the queue

    return data; // return the data
}

void queue_clean(Queue *queue) {
    for (uint32_t *begin = queue->serial_data, *end = begin + QUEUE_MAX_SIZE; begin < end; begin++) {
        begin = 0; // clear the queue
    }
}