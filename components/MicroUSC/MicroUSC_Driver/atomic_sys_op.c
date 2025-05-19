#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <atomic_sys_op.h>
#include <stdbool.h>

void queue_add(Queue *queue, const uint32_t data) {
    portENTER_CRITICAL(&queue->queueLock); // Lock the queue

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

    portEXIT_CRITICAL(&queue->queueLock); // Unlock the queue
}

uint32_t queue_top(Queue *queue) {
    portENTER_CRITICAL(&queue->queueLock); // Lock the queue
    uint_fast32_t head = queue->head; // get the current index
    const uint32_t data = queue->serial_data[head]; // get the data from the queue
    if (data == 0) {
        return 0; // queue is empty
    }

    queue->serial_data[head] = 0; //  // clear the data in the queue

    head = (head < QUEUE_MAX_SIZE) * (head + 1); // reset the index if out of bounds

    portEXIT_CRITICAL(&queue->queueLock); // Unlock the queue

    return data; // return the data
}

void queue_clean(Queue *queue) {
    portENTER_CRITICAL(&queue->queueLock); // Lock the queue
    memset(queue->serial_data, 0, QUEUE_MAX_SIZE);
    queue->head = 0;
    queue->tail = 9;
    portEXIT_CRITICAL(&queue->queueLock); // Unlock the queue
}