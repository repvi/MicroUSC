/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file initState.h
 * @brief Driver and task initialization state management for the MicroUSC system on ESP32/ESP8266.
 *
 * This header provides core functions and definitions for initializing, configuring, and managing
 * the state of driver structures and associated tasks within the MicroUSC library. It is designed
 * for robust embedded system startup, dynamic driver registration, task scheduling, and safe 
 * hardware interface management on ESP32/ESP8266 platforms.
 *
 * The API includes routines for:
 *   - Setting default driver and task configurations
 *   - Adding, removing, and freeing driver instances from system-managed lists
 *   - Managing task state, handlers, and default parameters
 *   - Deactivating drivers and ensuring safe shutdown or reconfiguration
 *
 * Organization and usage:
 *   - Functions are intended to be called during system initialization or reconfiguration,
 *     typically at startup or when dynamically managing drivers in a FreeRTOS environment.
 *   - The file is part of a modular component structure, supporting clear separation of concerns
 *     and maintainable C code organization using CMake and ESP-IDF build systems.
 *
 * @note Proper use of these routines is essential for preventing undefined behavior,
 *       memory leaks, or hardware conflicts in embedded applications.
 *       Manual file management and explicit initialization are recommended for clarity and reliability.
 *
 * @author Alejandro Ramirez
 * @date 5/26/2026
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "MicroUSC/system/memory_pool.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/synced_driver/atomic_sys_op.h"
#include "MicroUSC/internal/uscdef.h"
#include "MicroUSC/uscUniversal.h"
#include "genList.h"
#include "esp_err.h"

#ifdef LIST_HEAD
#undef LIST_HEAD
#endif

struct usc_driverList {
    struct usc_driver_t driver;
    struct list_head list;
};
 
struct usc_driversHandler {
    struct usc_driverList driver_list;
    size_t size;
    size_t max; // need to initial with size already, do not change size
    SemaphoreHandle_t lock;
};

struct usc_serialStorage {
    SerialDataQueueHandler serial_queue;
    SemaphoreHandle_t lock;
};

extern struct usc_driversHandler driver_system;

void driver_isr_trigger(struct usc_driver_t *driver);

/**
 * @brief Add a driver to the MicroUSC system with a specified FreeRTOS priority.
 *
 * Inserts a driver into the system's linked list of active drivers (using list_add_tail()),
 * assigning its execution priority for task scheduling. Ensures thread-safe access
 * to the driver list in FreeRTOS-based ESP32/ESP8266 projects.
 *
 * @param driver   Pre-initialized usc_driver_t structure (UART, etc.)
 * @param priority FreeRTOS task priority (0 <= priority <= configMAX_PRIORITIES-1)
 *
 * @note Duplicate driver additions are not checked; ensure uniqueness to avoid conflicts.
 */
void addSingleDriver( const char *const driver_name,
                      const uart_config_t uart_config,
                      const uart_port_config_t port_config,
                      const usc_process_t driver_process,
                      const stack_size_t stack_size
                    );
                    
/**
 * @brief Remove a single driver from the MicroUSC system.
 *
 * Safely unlinks the driver from the active list (using list_del()) and frees
 * associated resources. Designed for use with ESP32 dynamic driver configurations.
 *
 * @param item Pointer to the usc_driverList node containing the driver
 *
 * @warning Does not deinitialize hardware interfaces - call driver-specific deinit first.
 */
void removeSingleDriver(struct usc_driverList *item);

/**
 * @brief Clear all drivers from the MicroUSC system.
 *
 * Iterates through the driver list (using list_for_each_safe()), removes all entries,
 * and frees all allocated memory. Critical for preventing leaks during system shutdown
 * or reconfiguration in embedded applications.
 */
void freeDriverList(void);

#define WAIT_FOR_RESPONSE            ( pdMS_TO_TICKS( 1000 ) )

/**
 * @brief Initializes the memory pool for driver list nodes and their associated buffers.
 *
 * This function allocates a contiguous memory pool large enough to hold all driver list nodes,
 * their semaphores, and buffer memory, with proper alignment for each region. The pool is used
 * to efficiently manage memory for all drivers in the system, reducing fragmentation and
 * improving allocation speed.
 *
 * @param buffer_size The size (in bytes) of the buffer to allocate for each driver's buffer.memory.
 * @param data_size   The size (in bytes) of the data buffer (not currently used in this function, but stored for later use).
 * @return ESP_OK on success, ESP_ERR_NO_MEM if allocation fails.
 *
 * Example usage:
 *     init_driver_list_memory_pool(128, 256); // Allocates pool for DRIVER_MAX nodes, each with 128-byte buffer
 */
esp_err_t init_driver_list_memory_pool(const size_t buffer_size, const size_t data_size);

/**
 * @brief Initializes a static memory pool for driver task stacks.
 *
 * This function allocates a memory pool large enough to hold the stack for each driver task,
 * with each stack having the specified size. The pool is used to provide stack memory for
 * driver processor tasks, improving memory efficiency and allowing for static allocation.
 *
 * @param size The size (in bytes) of each task stack to allocate for every driver.
 * @return ESP_OK on success, ESP_ERR_NO_MEM if allocation fails.
 *
 * Example usage:
 *     setUSCtaskSize(2048); // Allocates a pool for DRIVER_MAX stacks, each 2048 bytes
 */
esp_err_t setUSCtaskSize(stack_size_t size);

/*
 * Wrapper for init_driver_list_memory_pool function
 */
esp_err_t init_hidden_driver_lists(const size_t buffer_size,  const size_t data_size);

/**
 * @brief Pause all USC driver tasks
 * 
 * Suspends both processor and reader tasks for all drivers in the list.
 * Function runs from IRAM for faster execution during interrupts.
 * 
 * @warning Not thread-safe - should be called with proper synchronization
 */
void IRAM_ATTR usc_drivers_pause(void);

/**
 * @brief Resume all USC driver tasks
 * 
 * Resumes both processor and reader tasks for all drivers in the list.
 * Function runs from IRAM for faster execution during interrupts.
 * 
 * @warning Not thread-safe - should be called with proper synchronization
 */
void IRAM_ATTR usc_drivers_resume(void);

#ifdef __cplusplus
}
#endif