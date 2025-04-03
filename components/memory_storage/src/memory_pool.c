#include "memory_pool.h"
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static bool set_memory_pool_vals(memory_pool_t *pool, const size_t block_size, const size_t num_blocks) {
    if (!pool || block_size == 0 || num_blocks == 0 || !pool->memory) {
        // Handle invalid input
        return false;
    }

    pool->block_size = block_size;
    pool->num_blocks = num_blocks;
    pool->free_blocks = num_blocks;

    pool->memory_free = pool->memory;

    memory_block_t *current = pool->memory_free;
    for (size_t i = 1; i < num_blocks; i++) {
        // Calculate the next block's address by adding block_size bytes
        // (uint8_t *) makes sure each increment is times 1 byte because uint8_t is one byte of memory
        current->next = (memory_block_t *)((uint8_t *)current + block_size); // do not touch
        current = current->next;
    }
    current->next = NULL; // Mark the end of the linked list

    return true;
}

static bool memory_pool_configure(memory_pool_t *pool, const size_t block_size, const size_t num_blocks) {
    const size_t total_size = block_size * num_blocks;

    pool->memory = heap_caps_malloc_prefer(total_size, 2,  MALLOC_CAP_32BIT, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (pool->memory == NULL) {
        return false;
    }

    return set_memory_pool_vals(pool, block_size, num_blocks); // can be optimized here
}

bool memory_pool_init(memory_pool_t *pool, const size_t block_size, const size_t num_blocks) {
    if (!pool || block_size == 0 || num_blocks == 0) {
        return false;
    }

    return memory_pool_configure(pool, block_size, num_blocks);
}

memory_pool_t *memory_pool_malloc(const size_t block_size, const size_t num_blocks) {
    if (block_size == 0 || num_blocks == 0) {
        return NULL;
    }

    memory_pool_t *pool = heap_caps_malloc_prefer(sizeof(memory_pool_t), 2,  MALLOC_CAP_32BIT, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (!pool) {
        return NULL;
    }

    if (!memory_pool_configure(pool, block_size, num_blocks)) {
        heap_caps_free(pool); // unable to create the memory for it
        pool = NULL;
        return pool;
    }

    return pool;
}

void *memory_pool_alloc(memory_pool_t *pool) {
    if (!pool || pool->memory_free == NULL) return NULL;

    memory_block_t *chunk = pool->memory_free; // actual memory

    pool->memory_free = chunk->next; // next memory available
    pool->free_blocks--;

    return (void *)chunk;
}

void memory_pool_free(memory_pool_t *pool, void *block) {
    if (!pool || !block) {
        return;
    }

    memory_block_t *chunk = block; // might need cast
    chunk->next = pool->memory_free;
    pool->memory_free = chunk;
    pool->free_blocks++;
}

void memory_pool_destroy(memory_pool_t *pool) {
    if (pool->memory) {
        heap_caps_free(pool->memory);
        pool->memory = NULL;
        pool->memory_free = NULL;
    }
    heap_caps_free(pool);
    pool = NULL;
}

void queue_add(memory_pool_t *pool, Queue *queue) {
    memory_block_t *new_block = (memory_block_t *)memory_pool_alloc(pool);
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
}

void queue_remove(memory_pool_t *pool, Queue *queue) {
    if (queue->head != NULL) {
        void *temp = (void *)queue->head;
        queue->head = queue->head->next;
        memory_pool_free(pool, temp);
    }
}

char *queue_top(memory_pool_t *pool, Queue *queue) {
    if (queue->head != NULL) {
        char *data = (char *)queue->head;
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