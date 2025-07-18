/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file atomic_sys_op.h
 * @brief Atomic data queue operations for ESP32/ESP8266 embedded systems
 *
 * Provides thread-safe data storage queues using FreeRTOS spinlocks for atomic access.
 * Designed for interrupt-safe data sharing between tasks and ISRs in MicroUSC drivers.
 *
 * Features:
 * - Atomic operations: Uses `portMUX_TYPE` spinlocks for critical sections
 * - Fixed-size queues: Preallocated buffers prevent heap fragmentation
 * - ISR-safe: Suitable for use in interrupt handlers when configured properly
 *
 * Usage:
 * 1. Create queue with `createDataStorageQueue()`
 * 2. Add/retrieve data using atomic operations
 * 3. Destroy with `destroyDataStorageQueue()` during cleanup
 *
 * @note The DataStorageQueue struct implementation is hidden in the .c file for encapsulation.
 *
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Opaque Handle Types --- */
typedef struct DataStorageQueue DataStorageQueue;
typedef DataStorageQueue *SerialDataQueueHandler;

/* --- API Functions --- */

size_t getDataStorageQueueSize(void);

/**
 * @brief Create thread-safe data queue with fixed capacity
 * @param len Maximum number of uint32_t elements
 * @return SerialDataQueueHandler Initialized queue handle, NULL on failure
 *
 * @note Allocates memory using ESP-IDF's heap_caps_malloc with MALLOC_CAP_8BIT
 * @warning Caller must destroy queue to prevent leaks
 */
SerialDataQueueHandler createDataStorageQueue(const size_t len);

SerialDataQueueHandler createDataStorageQueueStatic(void *buffer, const size_t serial_data_size);

/**
 * @brief Add data to queue (thread-safe)
 * @param queue Initialized queue handle
 * @param data 32-bit value to store
 *
 * @note Uses portENTER_CRITICAL for atomic access
 * @warning Blocks indefinitely if queue full - use in ISR with caution
 */
void dataStorageQueue_add(SerialDataQueueHandler queue, const uint32_t data);

/**
 * @brief Retrieve oldest data from queue (thread-safe)
 * @param queue Initialized queue handle
 * @return uint32_t Retrieved value (0 if empty)
 *
 * @note Uses portENTER_CRITICAL for atomic access
 */
uint32_t dataStorageQueue_top(SerialDataQueueHandler queue);

/**
 * @brief Reset queue to empty state (thread-safe)
 * @param queue Initialized queue handle
 *
 * @note Does not deallocate memory - queue remains usable
 */
void dataStorageQueue_clean(SerialDataQueueHandler queue);

/**
 * @brief Destroy queue and release resources
 * @param queue Valid queue handle from createDataStorageQueue()
 *
 * @note Frees both queue structure and data buffer
 */
void destroyDataStorageQueue(SerialDataQueueHandler queue);

#ifdef __cplusplus
}
#endif