/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file memory_pool.h
 * @brief IRAM-optimized memory pool management for ESP32/ESP8266 embedded systems
 * 
 * Provides fixed-size block memory allocation with ESP32-specific optimizations,
 * including IRAM placement and cache-friendly alignment. Designed for deterministic
 * memory access in FreeRTOS environments and interrupt handlers.
 *
 * Features:
 * - Preallocated memory pools to prevent fragmentation in constrained environments
 * - Thread-safe operations for FreeRTOS task and ISR contexts
 * - Explicit memory placement control (DRAM, PSRAM, IRAM)
 * - O(1) allocation/deallocation for real-time performance
 *
 * Usage:
 * 1. Initialize pool with memory_pool_init() or memory_pool_malloc()
 * 2. Allocate blocks with memory_pool_alloc()
 * 3. Release blocks with memory_pool_free()
 * 4. Destroy pools with memory_pool_destroy() during cleanup
 *
 * @note Part of MicroUSC library's memory management subsystem
 * 
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

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
 * @brief Initialize a memory pool for efficient block allocation.
 * 
 * This function sets up a fixed-size block memory pool preallocated in IRAM, 
 * optimized for ESP32/ESP8266 memory constraints. Proper alignment is enforced
 * to prevent fragmentation and ensure cache-friendly access patterns.
 * 
 * @param pool Pointer to pre-allocated memory_pool_t structure
 * @param block_size Size of each memory block (bytes). Must be ≥ sizeof(void*).
 * @param num_blocks Total blocks in pool. Determines total pool size.
 * @return true - Pool initialized successfully
 * @return false - Invalid parameters or allocation failure
 * 
 * @note Must be called once before any memory pool operations. 
 *       Subsequent calls on initialized pools cause undefined behavior.
 *       Use IRAM_ATTR if pool will be accessed from interrupts.
 */
bool memory_pool_init(memory_pool_t *pool, const size_t block_size, const size_t num_blocks);

/**
 * @brief Dynamically allocate and initialize a memory pool in IRAM.
 * 
 * This function creates a new memory pool structure and its associated blocks in IRAM,
 * optimized for ESP32/ESP8266 memory constraints with proper alignment to prevent fragmentation.
 * Combines allocation and initialization into a single step for convenience.
 * 
 * @param block_size Size of each memory block (bytes). Must be ≥ sizeof(void*).
 * @param num_blocks Total blocks in pool. Determines total pool size.
 * @return memory_pool_t* - Pointer to initialized pool, or NULL on failure
 * 
 * @note Caller must free with memory_pool_free() to avoid leaks.
 *       Designed for cache-friendly access patterns and ISR safety when using IRAM_ATTR.
 *       Prefer this over manual initialization for dynamic pool management.
 */
memory_pool_t *memory_pool_malloc(const size_t block_size, const size_t num_blocks)  __attribute__((malloc));

#define memory_handler_malloc(block_size, num_blocks) memory_pool_malloc(block_size, num_blocks);

/**
 * @brief Allocate a memory block from the memory pool.
 *
 * This function returns a pointer to a free memory block from the specified memory pool.
 * If no blocks are available, it returns NULL.
 * The allocation is optimized for ESP32/ESP8266 and embedded systems, ensuring efficient use of IRAM and minimal fragmentation.
 *
 * @param pool Pointer to an initialized memory_pool_t structure.
 * @return void* Pointer to a free memory block, or NULL if the pool is exhausted.
 *
 * @note The returned pointer must not be freed directly; use memory_pool_free_block() or an equivalent pool-specific release function.
 *       Suitable for use in embedded and real-time applications where deterministic memory allocation is required.
 */
void *memory_pool_alloc(memory_pool_t *pool)  __attribute__((malloc));

/**
 * @brief Return a memory block to the memory pool.
 *
 * This function releases a previously allocated memory block back to the specified memory pool,
 * making it available for future allocations. It is designed for efficient memory management
 * in embedded systems and ensures minimal fragmentation when used with preallocated memory pools.
 *
 * @param pool  Pointer to an initialized memory_pool_t structure.
 * @param block Pointer to the memory block to be returned to the pool.
 *
 * @note The block must have been allocated from the same pool using memory_pool_alloc().
 *       This function is safe for use in real-time and FreeRTOS-based applications on ESP32/ESP8266.
 */
void memory_pool_free(memory_pool_t *pool, void *block);

/**
 * @brief Destroy a memory pool and release all associated resources.
 *
 * This function deallocates all memory and internal structures associated with the specified memory pool,
 * ensuring that no memory leaks occur and that all resources are properly cleaned up.
 * After calling this function, the memory_pool_t pointer and any blocks previously allocated from the pool
 * must not be used.
 *
 * @param pool Pointer to the memory_pool_t structure to be destroyed.
 *
 * @note This function should be called when the memory pool is no longer needed, typically during
 *       system shutdown or module cleanup in embedded and ESP32/ESP8266 applications.
 *       Do not use the pool or any of its blocks after destruction to avoid undefined behavior.
 */
void memory_pool_destroy(memory_pool_t* pool);

#ifdef __cplusplus
}
#endif