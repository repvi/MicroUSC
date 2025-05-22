#pragma once

#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include <stdatomic.h>
#include "MicroUSC/internal/USC_driver_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QUEUE_MAX_SIZE       256 // Maximum size of the queue

typedef struct {
    uint32_t serial_data[QUEUE_MAX_SIZE];
    portMUX_TYPE critical_lock;
    uint_fast32_t head;
    uint_fast32_t tail;
} Queue;

void queue_add(Queue *queue, const uint32_t data);

uint32_t queue_top(Queue *queue);

void queue_clean(Queue *queue);

#ifdef __cplusplus
}
#endif