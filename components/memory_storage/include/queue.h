#pragma once

#include "memory_pool.h"

typedef struct QueueNode {
    char data[15];
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *head;
    QueueNode *tail;
    size_t count;
} Queue;

void queue_add(memory_pool_t *pool, Queue *queue);

void queue_remove(memory_pool_t *pool, Queue *queue);

char *queue_top(memory_pool_t *pool, Queue *queue);

void queue_delete(memory_pool_t *pool, Queue *queue);

void queue_init(Queue *q, memory_pool_t *pool);
bool queue_enqueue(Queue *q, const char *data);
bool queue_dequeue(Queue *q, char *out_data);
