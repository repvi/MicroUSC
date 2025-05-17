#ifndef __MEMORY_POOL_H
#define __MEMORY_POOL_H

#include "esp_system.h"
#include "esp_heap_caps.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef char serial_data_t[15];

// Might not be used in main program
typedef enum {
    DRAM = MALLOC_CAP_8BIT,
    PSRAM = MALLOC_CAP_SPIRAM,
    IRAM = MALLOC_CAP_32BIT,
} memory_placement_t;

typedef struct memory_block_t {
    struct memory_block_t *next; // next element in the block
} memory_block_t;

/**
 * Struct representing a memory pool.
 */
typedef struct {
    memory_block_t *memory_free; // all the available memory in the memory pool
    void *memory;
    size_t block_size;     // Size of each block in the pool
    size_t num_blocks;     // Total number of blocks in the pool
    size_t free_blocks;    // This isn't being used in anything so far>>>>>>>
} memory_pool_t;

typedef memory_pool_t *memory_block_handle_t;

/**
 * Initializes a memory pool.
 * 
 * @param pool Pointer to the memory_pool_t structure.
 * @param block_size Size of each block in bytes.
 * @param num_blocks Number of blocks in the pool.
 * @return True if initialization succeeds, false otherwise.
 */
bool memory_pool_init(memory_pool_t *pool, const size_t block_size, const size_t num_blocks);

memory_pool_t *memory_pool_malloc(const size_t block_size, const size_t num_blocks)  __attribute__((malloc));
/**
 * Allocates a block from the memory pool.
 * 
 * @param pool Pointer to the memory_pool_t structure.
 * @return Pointer to the allocated block, or NULL if no blocks are available.
 */
void *memory_pool_alloc(memory_pool_t *pool)  __attribute__((malloc));

/**
 * Frees a block back to the memory pool.
 * 
 * @param pool Pointer to the memory_pool_t structure.
 * @param block Pointer to the block to be memory_free.
 */
void memory_pool_free(memory_pool_t *pool, void *block);

/**
 * Destroys a memory pool and frees all associated resources.
 * 
 * @param pool Pointer to the memory_pool_t structure.
 */
void memory_pool_destroy(memory_pool_t* pool);


#define memory_handler_malloc(block_size, num_blocks) memory_pool_malloc(block_size, num_blocks);




#ifdef __cplusplus
}
#endif

#endif // MEMORY_POOL_H