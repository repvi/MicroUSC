#ifndef ATOMIC_QUEUE_H
#define ATOMIC_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include "USC_driver_config.h"

#define QUEUE_MAX_SIZE (1 << 4) // Maximum size of the queue

typedef struct {
    uint32_t serial_data[QUEUE_MAX_SIZE];
    uint_fast32_t head;
    uint_fast32_t tail;
} Queue;

void queue_add(Queue *queue, const uint32_t data);

uint32_t queue_top(Queue *queue);

void queue_clean(Queue *queue);

#ifdef __cplusplus
}
#endif

#endif