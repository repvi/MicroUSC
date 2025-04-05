#include "queue.h"

void queue_add(memory_pool_t *pool, Queue *queue) {
    QueueNode *new_block = (QueueNode *)memory_pool_alloc(pool);
    if (new_block == NULL) {
        return; // not allocated
    }

    if (queue->head != NULL) {
        queue->tail->next = new_block;
        queue->tail = queue->tail->next;
    }
    else {
        queue->head = new_block;
        queue->tail = queue->head;
    }

    queue->count++;
}

void queue_remove(memory_pool_t *pool, Queue *queue) {
    if (queue->head != NULL) {
        void *temp = (void *)queue->head;
        queue->head = queue->head->next;
        memory_pool_free(pool, temp);
        queue->count--;
    }
}

char *queue_top(memory_pool_t *pool, Queue *queue) {
    if (queue->head != NULL) {
        char *data = (char *)queue->head; // already casts it to a char
        queue_remove(pool, queue);
        return data;
    }
    else {
        return NULL;
    }
}

void queue_delete(memory_pool_t *pool, Queue *queue) {
    while (queue->head != NULL) {
        queue_remove(pool, queue);   
    }
}